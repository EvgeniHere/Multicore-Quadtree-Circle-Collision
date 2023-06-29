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
int tag_numCells = 4;
int tag_cells = 5;

int frames = 0;
clock_t begin;

int rank, size;

int numProcesses;

int numAllCircles;
struct Circle* allCircles;

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
    int numCells;
    int* circle_ids;
    struct Circle* circles;
    struct Rectangle* rects;
};

struct Process* processes;
struct Process* curProcess;

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 1) {
        fprintf(stderr, "This program requires at least 1 process.\n");
        MPI_Finalize();
        return 1;
    }

    numProcesses = size;
    numAllCircles = 100;
    circleSize = 50.0;
    maxSpeed = 1.0;
    maxCirclesPerCell = 3;
    minCellSize = 2 * circleSize + 2 * maxSpeed;
    circle_max_X = SCREEN_WIDTH;
    circle_max_y = SCREEN_HEIGHT;

    numCircles = numAllCircles/numProcesses;
    circles = (struct Circle *) malloc(sizeof(struct Circle) * numCircles);
    allCircles = (struct Circle *) malloc(sizeof(struct Circle) * numAllCircles);

    if (rank == 0) {
        srand(90);
        for (int i = 0; i < numAllCircles; i++) {
            allCircles[i].posX = random_double(circleSize / 2.0, SCREEN_WIDTH - circleSize / 2.0);
            allCircles[i].posY = random_double(circleSize / 2.0, SCREEN_HEIGHT - circleSize / 2.0);
            allCircles[i].velX = random_double(-maxSpeed, maxSpeed);
            allCircles[i].velY = random_double(-maxSpeed, maxSpeed);
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
            processes[i].circles = (struct Circle *) malloc(numCircles * sizeof(struct Circle));
            processes[i].rects = (struct Rectangle *) malloc(sizeof(struct Rectangle));
            processes[i].circle_ids = (int *) malloc(numCircles * sizeof(int));
            processes[i].numCells = 0;
            if (i > 0)
                MPI_Send(&processes[i], sizeof(struct Process), MPI_BYTE, i, tag_process, MPI_COMM_WORLD);
        }
        distributeCircles();
        setupQuadtree(processes[0].posX, processes[0].posY, processes[0].width, processes[0].height);
    } else {
        curProcess = (struct Process*) malloc(sizeof(struct Process));
        MPI_Recv(curProcess, sizeof(struct Process), MPI_BYTE, 0, tag_process, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&numCircles, 1, MPI_INT, 0, tag_numCircles, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        circles = (struct Circle*) realloc(circles, numCircles * sizeof(struct Circle)); // FU*K
        MPI_Recv(circles, numCircles * sizeof(struct Circle), MPI_BYTE, 0, tag_circles, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        setupQuadtree(curProcess->posX, curProcess->posY, curProcess->width, curProcess->height);
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
    updateTree();
    if (rank != 0) {
        //MPI_Send(&numCells, 1, MPI_INT, 0, tag_numCells, MPI_COMM_WORLD);
        //MPI_Send(leaf_rects, numCells * sizeof(struct Rectangle), MPI_BYTE, 0, tag_cells, MPI_COMM_WORLD);
        circles = (struct Circle*) realloc(circles, numCircles * sizeof(struct Circle));
    } else {
        processes[0].circles = circles;
        //processes[0].numCells = numCells;
        //processes[0].rects = (struct Rectangle*) realloc(processes[0].rects, processes[0].numCells * sizeof(struct Rectangle));
        /*for (int i = 0; i < numCells; i++) {
            struct Rectangle* rect = (struct Rectangle*) malloc(sizeof(struct Rectangle));
            rect->posX = leaf_rects[i].posX;
            rect->posY = leaf_rects[i].posY;
            rect->width = leaf_rects[i].width;
            rect->height = leaf_rects[i].height;
            processes[0].rects[i] = *rect;
        }*/
        for (int i = 0; i < numProcesses; i++) {
            //MPI_Recv(&processes[i].numCells, 1, MPI_INT, i, tag_numCells, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            //processes[i].rects = (struct Rectangle*) realloc(processes[i].rects, processes[i].numCells * sizeof(struct Rectangle));
            //MPI_Recv(processes[i].rects, processes[i].numCells * sizeof(struct Rectangle), MPI_BYTE, i, tag_cells, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (i > 0)
                MPI_Recv(processes[i].circles, numCircles * sizeof(struct Circle), MPI_BYTE, i, tag_circles, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int j = 0; j < numCircles; j++) {
                allCircles[processes[i].circle_ids[j]] = processes[i].circles[j];
            }
        }
    }

    MPI_Bcast(circles, numCircles, MPI_BYTE, 0, MPI_COMM_WORLD);

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

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, SCREEN_WIDTH, 0, SCREEN_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    //glPointSize(circleSize);

    for (int i = 0; i < numProcesses; i++) {
        glLineWidth(1);
        glColor3f(255, 255, 255);
        for (int j = 0; j < processes[i].numCells; j++) {
            struct Rectangle* rect = &processes[i].rects[j];
            glBegin(GL_LINE_LOOP);
            glVertex2f(rect->posX + 1, rect->posY + 1);
            glVertex2f(rect->posX + 1, rect->posY + 1 + rect->height - 2);
            glVertex2f(rect->posX + 1 + rect->width - 2, rect->posY + 1 + rect->height - 2);
            glVertex2f(rect->posX + 1 + rect->width - 2, rect->posY + 1);
            glEnd();
        }
        glLineWidth(5);
        glColor3f(255, 0, 0);
        glBegin(GL_LINE_LOOP);
        glVertex2f(processes[i].posX + 1, processes[i].posY + 1);
        glVertex2f(processes[i].posX + 1, processes[i].posY + 1 + processes[i].height - 2);
        glVertex2f(processes[i].posX + 1 + processes[i].width - 2, processes[i].posY + 1 + processes[i].height - 2);
        glVertex2f(processes[i].posX + 1 + processes[i].width - 2, processes[i].posY + 1);
        glEnd();
    }

    glColor3f(255, 255, 255);
    for (int i = 0; i < numAllCircles; i++) {
        drawCircle(allCircles[i].posX, allCircles[i].posY);
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
        for (int i = 0; i < 6; i++) {
            GLfloat angle = i * (2.0 * M_PI / 6);
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
