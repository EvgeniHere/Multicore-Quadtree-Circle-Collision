#include <stdlib.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <mpi.h>
#include "window.c"
#include "quadtree.c"
#include <GL/freeglut.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 1000

int tag_circles = 1;
int tag_numCircles = 2;
int tag_process = 3;

int frames = 0;
clock_t begin;

int rank, size;

int numProcesses;

int main(int argc, char** argv);
void update();
void display();
void drawCircle(GLfloat centerX, GLfloat centerY);
void initOpenGL(int* argc, char** argv);
void distributeCircles();
int sleep(long ms);

struct Process {
    int posX;
    int posY;
    int width;
    int height;
    int numCircles;
    int* circle_ids;
    struct Circle* circles;
};

struct Process* processes;
struct Process* curProcess;

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        fprintf(stderr, "This program requires at least 5 processes.\n");
        MPI_Finalize();
        return 1;
    }

    numProcesses = size - 1;

    numCircles = 100000;
    circleSize = 1.0;
    maxSpeed = circleSize / 2.0;
    maxCirclesPerCell = 3;
    minCellSize = 2 * circleSize + 4 * maxSpeed;
    circle_max_X = SCREEN_WIDTH;
    circle_max_y = SCREEN_HEIGHT;

    circles = (struct Circle *) malloc(sizeof(struct Circle) * numCircles);

    if (rank == 0) {
        srand(90);
        for (int i = 0; i < numCircles; i++) {
            circles[i].posX = random_double(circleSize / 2.0, SCREEN_WIDTH - circleSize / 2.0);
            circles[i].posY = random_double(circleSize / 2.0, SCREEN_HEIGHT - circleSize / 2.0);
            circles[i].velX = random_double(-maxSpeed, maxSpeed);
            circles[i].velY = random_double(-maxSpeed, maxSpeed);
        }

        processes = (struct Process*) malloc(numProcesses * sizeof(struct Process));

        int numColumns = (int)ceil(sqrt(numProcesses));
        int numRows = (int)ceil((float)numProcesses / numColumns);

        int rectWidth = SCREEN_WIDTH / numColumns;
        int rectHeight = SCREEN_HEIGHT / numRows;

        int rectIndex = 0;
        for (int y = 0; y < numRows; y++) {
            for (int x = 0; x < numColumns; x++) {
                if (rectIndex >= numProcesses) {
                    break;
                }
                if (y == numRows - 1 && x == 0 && rectIndex + numColumns >= numProcesses) {
                    rectWidth = SCREEN_WIDTH / (numProcesses - rectIndex);
                }
                processes[rectIndex].posX = x * rectWidth;
                processes[rectIndex].posY = y * rectHeight;
                processes[rectIndex].width = rectWidth;
                processes[rectIndex].height = rectHeight;
                rectIndex++;
            }
        }

        for (int i = 0; i < numProcesses; i++) {
            processes[i].circles = (struct Circle *) malloc(processes[i].numCircles * sizeof(struct Circle));
            processes[i].circle_ids = (int *) malloc(processes[i].numCircles * sizeof(int));
            processes[i].numCircles = 0;
            MPI_Send(&processes[i], sizeof(struct Process), MPI_BYTE, i + 1, tag_process, MPI_COMM_WORLD);
        }

        distributeCircles();
    } else {
        curProcess = (struct Process*) malloc(sizeof(struct Process));
        MPI_Recv(curProcess, sizeof(struct Process), MPI_BYTE, 0, tag_process, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        treePosX = curProcess->posX;
        treePosY = curProcess->posY;
        treeWidth = curProcess->width;
        treeHeight = curProcess->height;
    }

    if (rank == 0) {
        glutInit((int *) &argc, argv);
        begin = clock();
        initOpenGL(&argc, argv);
    } else {
        while (true) {
            update();
        }
    }

    MPI_Finalize();
    return 0;
}

void update() {
    if (rank != 0) {
        MPI_Recv(&numCircles, 1, MPI_INT, 0, tag_numCircles, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        circles = (struct Circle*) realloc(circles, numCircles * sizeof(struct Circle)); // FU*K
        MPI_Recv(circles, numCircles * sizeof(struct Circle), MPI_BYTE, 0, tag_circles, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        rebuildTree();
        MPI_Send(circles, numCircles * sizeof(struct Circle), MPI_BYTE, 0, tag_circles, MPI_COMM_WORLD);
    } else {
        for (int i = 0; i < numProcesses; i++) {
            MPI_Recv(processes[i].circles, processes[i].numCircles * sizeof(struct Circle), MPI_BYTE, i + 1, tag_circles, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int j = 0; j < processes[i].numCircles; j++) {
                circles[processes[i].circle_ids[j]] = processes[i].circles[j];
            }
        }
        distributeCircles();
    }

    if (rank == 0) {
        frames++;
        clock_t end = clock();
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        if (time_spent >= 10) {
            printf("%d frames for 10 seconds\n", frames);
            printf("%f FPS\n", frames / 10.0);
            frames = 0;
            begin = end;
            //MPI_Finalize();
            //exit(0);
        }
        glutPostRedisplay();
        glutTimerFunc(0, update, 0);
    }
}

void distributeCircles() {
    if (rank == 0) {
        for (int i = 0; i < numProcesses; i++) {
            processes[i].numCircles = 0;
        }

        for (int i = 0; i < numCircles; i++) {
            int process_index = 0;
            for (int j = 0; j < numProcesses; j++) {
                if (!isCircleOverlappingArea(&circles[i], processes[j].posX, processes[j].posY, processes[j].width, processes[j].height))
                    continue;
                process_index = j;
                break;
            }
            processes[process_index].numCircles++;
        }

        for (int i = 0; i < numProcesses; i++) {
            processes[i].circles = (struct Circle *) realloc(processes[i].circles, processes[i].numCircles * sizeof(struct Circle));
            processes[i].circle_ids = (int *) realloc(processes[i].circle_ids, processes[i].numCircles * sizeof(int));
            processes[i].numCircles = 0;
        }

        for (int i = 0; i < numCircles; i++) {
            int process_index = 0;
            for (int j = 0; j < numProcesses; j++) {
                if (!isCircleOverlappingArea(&circles[i], processes[j].posX, processes[j].posY, processes[j].width, processes[j].height))
                    continue;
                process_index = j;
                break;
            }
            processes[process_index].circles[processes[process_index].numCircles] = circles[i];
            processes[process_index].circle_ids[processes[process_index].numCircles] = i;
            processes[process_index].numCircles++;
        }
        for (int i = 0; i < numProcesses; i++) {
            MPI_Send(&processes[i].numCircles, 1, MPI_INT, i + 1, tag_numCircles, MPI_COMM_WORLD);
            MPI_Send(processes[i].circles, processes[i].numCircles * sizeof(struct Circle), MPI_BYTE, i + 1, tag_circles, MPI_COMM_WORLD);
        }
    }
}

void distributeCircles2() {
    for (int i = 0; i < numProcesses; i++) {
        MPI_Recv(processes[i].circles, processes[i].numCircles * sizeof(struct Circle), MPI_BYTE, i + 1, tag_circles, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        for (int j = 0; j < processes[i].numCircles; j++) {
            circles[processes[i].circle_ids[j]] = processes[i].circles[j];
            if (isCircleOverlappingArea(&processes[i].circles[j], processes[j].posX, processes[j].posY, processes[j].width, processes[j].height))
                continue;
            for (int k = 0; k < numProcesses; k++) {
                if (k == i)
                    continue;
                if (!isCircleOverlappingArea(&processes[i].circles[j], processes[k].posX, processes[k].posY, processes[k].width, processes[k].height))
                    continue;
                bool alreadyContains = false;
                for (int l = 0; l < processes[k].numCircles; l++) {
                    if (processes[k].circle_ids[l] != processes[i].circle_ids[j])
                        continue;
                    alreadyContains = true;
                    break;
                }
                if (alreadyContains)
                    continue;
                processes[k].numCircles++;
                processes[k].circles = (struct Circle*) realloc(processes[k].circles, processes[k].numCircles * sizeof(struct Circle));
                processes[k].circles[processes[k].numCircles-1] = processes[i].circles[j];
                processes[k].circle_ids[processes[k].numCircles-1] = processes[i].circle_ids[j];
            }
            for (int k = j; k < processes[i].numCircles-1; k++) {
                processes[i].circles[k] = processes[i].circles[k+1];
                processes[i].circle_ids[k] = processes[i].circle_ids[k+1];
            }
            processes[i].numCircles--;
        }
    }
    for (int i = 0; i < numProcesses; i++) {
        processes[i].circles = (struct Circle*) realloc(processes[i].circles, processes[i].numCircles * sizeof(struct Circle));
        processes[i].circle_ids = (int*) realloc(processes[i].circle_ids, processes[i].numCircles * sizeof(int));
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, SCREEN_WIDTH, 0, SCREEN_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    //glPointSize(circleSize);

    for (int i = 0; i < numProcesses; i++) {
        glBegin(GL_LINE_LOOP);
        glVertex2f(processes[i].posX + 1, processes[i].posY + 1);
        glVertex2f(processes[i].posX + 1, processes[i].posY + 1 + processes[i].height - 2);
        glVertex2f(processes[i].posX + 1 + processes[i].width - 2, processes[i].posY + 1 + processes[i].height - 2);
        glVertex2f(processes[i].posX + 1 + processes[i].width - 2, processes[i].posY + 1);
        glEnd();
    }

    for (int i = 0; i < numCircles; i++) {
        drawCircle(circles[i].posX, circles[i].posY);
    }

    glutSwapBuffers();
}

void closeWindow() {
    MPI_Abort(MPI_COMM_WORLD, 0);
    MPI_Finalize();
    exit(0);
}

void initOpenGL(int* argc, char** argv) {
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutInitWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
    glutCreateWindow("HPC Bouncing Circles");
    glutCloseFunc(closeWindow);
    glutDisplayFunc(display);
    glutTimerFunc(0, update, 0);
    glutMainLoop();
}

void drawCircle(GLfloat centerX, GLfloat centerY) {
    if (circleSize <= 2.0f) {
        glBegin(GL_POINTS);
        glVertex2i(centerX, centerY);
        glEnd();
    } else {
        glBegin(GL_POLYGON);
        for (int i = 0; i < 32; i++) {
            GLfloat angle = i * (2.0 * M_PI / 32);
            GLfloat x = centerX + (circleSize/2) * cos(angle);
            GLfloat y = centerY + (circleSize/2) * sin(angle);
            glVertex2f(x, y);
        }
        glEnd();
    }
}


int sleep(long tms) {
    struct timespec ts;
    int ret;

    if (tms < 0) {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = tms / 1000;
    ts.tv_nsec = (tms % 1000) * 1000000;

    do {
        ret = nanosleep(&ts, &ts);
    } while (ret && errno == EINTR);

    return ret;
}
