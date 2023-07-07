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

int tag_circles = 1;
int tag_numCircles = 2;
int tag_process = 3;
int tag_numCells = 4;
int tag_cells = 5;
int tag_circle_inside = 6;
int tag_newNumCircles = 7;
int tag_newCircles = 8;

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

struct Process {
    int posX;
    int posY;
    int width;
    int height;
    int numCells;
    int numCircles;
    struct Circle* circles;
    struct Rectangle* rects;
};

struct Process* processes;
struct Process* curProcess;

int newNumCircles = 0;
struct Circle* tmpCircles;
struct Circle* newCircles;

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
    numCircles = 100000;
    circleSize = 1.0;
    maxSpeed = 1.0;
    maxCirclesPerCell = 100;
    minCellSize = 4 * circleSize;
    friction = 0.999;
    gravity = 0.000003;
    circle_max_X = SCREEN_WIDTH;
    circle_max_y = SCREEN_HEIGHT;

    circles = (struct Circle *) malloc(sizeof(struct Circle) * numCircles);
    circle_inside = (bool *) malloc(sizeof(bool) * numCircles);
    curProcess = (struct Process*) malloc(sizeof(struct Process));
    newCircles = (struct Circle*) malloc(newNumCircles * sizeof(struct Circle));

    if (rank == 0) {
        srand(90);
        for (int i = 0; i < numCircles; i++) {
            circles[i].id = i;
            circles[i].posX = random_double(circleSize / 2.0, SCREEN_WIDTH - circleSize / 2.0);
            circles[i].posY = random_double(circleSize / 2.0, SCREEN_HEIGHT - circleSize / 2.0);
            if (circles[i].posX < SCREEN_WIDTH/2.0 && circles[i].posY < SCREEN_HEIGHT/2.0) {
                circles[i].velX = 0;
                circles[i].velY = maxSpeed;//random_double(0.001, maxSpeed);
            } else if (circles[i].posX < SCREEN_WIDTH/2.0 && circles[i].posY >= SCREEN_HEIGHT/2.0) {
                circles[i].velX = maxSpeed;//random_double(0.001, maxSpeed);
                circles[i].velY = 0;
            } else if (circles[i].posX >= SCREEN_WIDTH/2.0 && circles[i].posY < SCREEN_HEIGHT/2.0) {
                circles[i].velX = -maxSpeed;//random_double(-0.001, -maxSpeed);
                circles[i].velY = 0;
            } else if (circles[i].posX >= SCREEN_WIDTH/2.0 && circles[i].posY >= SCREEN_HEIGHT/2.0) {
                circles[i].velX = 0;
                circles[i].velY = -maxSpeed;//random_double(-0.001, -maxSpeed);
            }
            //circles[i].velX = random_double(-maxSpeed, maxSpeed);
            //circles[i].velY = random_double(-maxSpeed, maxSpeed);
            //circles[i].mass = circles[i]->size;
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
            if (i > 0)
                MPI_Send(&processes[i], sizeof(struct Process), MPI_BYTE, i, tag_process, MPI_COMM_WORLD);
            processes[i].rects = (struct Rectangle *) malloc(sizeof(struct Rectangle));
            processes[i].circles = (struct Circle *) malloc(numCircles * sizeof(struct Circle));
            processes[i].numCells = 0;
            processes[i].numCircles = 0;
        }
        curProcess = &processes[0];
    } else {
        MPI_Recv(curProcess, sizeof(struct Process), MPI_BYTE, 0, tag_process, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    MPI_Bcast(circles, numCircles * sizeof(struct Circle), MPI_BYTE, 0, MPI_COMM_WORLD);
    setupQuadtree(curProcess->posX, curProcess->posY, curProcess->width, curProcess->height);

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

void distributeCircles() {
    if (rank == 0) {
        MPI_Request *requests = (MPI_Request *) malloc((numProcesses - 1) * sizeof(MPI_Request));
        MPI_Status *statuses = (MPI_Status *) malloc((numProcesses - 1) * sizeof(MPI_Status));
        for (int i = 0; i < numProcesses; i++) {
            processes[i].numCircles = 0;
            for (int j = 0; j < numCircles; j++) {
                if (isCircleOverlappingArea(&circles[j], processes[i].posX, processes[i].posY, processes[i].width,
                                            processes[i].height))
                    processes[i].numCircles++;
            }
            processes[i].circles = (struct Circle *) realloc(processes[i].circles, processes[i].numCircles * sizeof(struct Circle));
            processes[i].numCircles = 0;
            for (int j = 0; j < numCircles; j++) {
                if (isCircleOverlappingArea(&circles[j], processes[i].posX, processes[i].posY, processes[i].width,
                                            processes[i].height))
                    processes[i].circles[processes[i].numCircles++] = circles[j];
            }
            if (i > 0) {
                MPI_Request request;
                MPI_Isend(&processes[i].numCircles, 1, MPI_INT, i, tag_numCircles, MPI_COMM_WORLD, &request);
                MPI_Isend(processes[i].circles, processes[i].numCircles * sizeof(struct Circle), MPI_BYTE, i,
                          tag_circles, MPI_COMM_WORLD, &requests[i - 1]);
            }
        }
        newCircles = processes[0].circles;
        newNumCircles = processes[0].numCircles;
        MPI_Waitall(numProcesses - 1, requests, statuses);
        free(requests);
        free(statuses);
    } else {
        MPI_Request request1, request2;
        MPI_Status status1, status2;
        MPI_Irecv(&newNumCircles, 1, MPI_INT, 0, tag_numCircles, MPI_COMM_WORLD, &request1);
        MPI_Wait(&request1, &status1);
        newCircles = (struct Circle*) realloc(newCircles, newNumCircles * sizeof(struct Circle));
        MPI_Irecv(newCircles, newNumCircles * sizeof(struct Circle), MPI_BYTE, 0, tag_circles, MPI_COMM_WORLD, &request2);
        MPI_Wait(&request2, &status2);
        for (int i = 0; i < newNumCircles; i++) {
            circles[newCircles[i].id] = newCircles[i];
        }
    }
}

void update() {
    distributeCircles();

    updateTree();

    if (size > 1) {
        if (rank != 0) {
            MPI_Request request1, request2;
            MPI_Status status1, status2;
            for (int i = 0; i < newNumCircles; i++) {
                newCircles[i] = circles[newCircles[i].id];
            }
            MPI_Isend(&newNumCircles, 1, MPI_INT, 0, tag_newNumCircles, MPI_COMM_WORLD, &request1);
            MPI_Isend(newCircles, newNumCircles * sizeof(struct Circle), MPI_BYTE, 0, tag_newCircles, MPI_COMM_WORLD, &request2);
            MPI_Wait(&request1, &status1);
            MPI_Wait(&request2, &status2); // KANN WEG? !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        } else {
            MPI_Request *requests = (MPI_Request *) malloc((numProcesses - 1) * sizeof(MPI_Request));
            MPI_Status *statuses = (MPI_Status *) malloc((numProcesses - 1) * sizeof(MPI_Status));
            int num = 0;
            int source;
            for (int i = 1; i < numProcesses; i++) {
                MPI_Request request1;
                MPI_Status status1;
                MPI_Irecv(&num, 1, MPI_INT, MPI_ANY_SOURCE, tag_newNumCircles, MPI_COMM_WORLD, &request1);
                MPI_Wait(&request1, &status1);
                source = status1.MPI_SOURCE;
                processes[source].numCircles = num;
                processes[source].circles = (struct Circle*) realloc(processes[source].circles, processes[source].numCircles * sizeof(struct Circle));
                MPI_Irecv(processes[source].circles, numCircles * sizeof(struct Circle), MPI_BYTE, source,
                          tag_newCircles, MPI_COMM_WORLD, &requests[source-1]);
                MPI_Wait(&requests[source-1], &statuses[source-1]);
                for (int j = 0; j < processes[source].numCircles; j++) {
                    circles[processes[source].circles[j].id] = processes[source].circles[j];
                }
            }
            processes[0].circles = newCircles;
            free(requests);
            free(statuses);
        }
    }

    frames++;
    if (rank == 0) {
        clock_t end = clock();
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        if (time_spent >= 10) {
            //printf("%d frames for 10 seconds\n", frames);
            printf("%f FPS\n", frames / time_spent);
            frames = 0;
            begin = end;
            //MPI_Finalize();
            //exit(0);
            //printTree(rootCell, 0);
        }
        glutTimerFunc(0, update, 0);
        if (frames % 2 == 0)
            glutPostRedisplay();
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

    /*for (int i = 0; i < numProcesses; i++) {
        /*glLineWidth(1);
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
    }*/

    glColor3f(255, 255, 255);
    for (int i = 0; i < numProcesses; i++) {
        for (int j = 0; j < processes[i].numCircles; j++) {
            drawCircle(processes[i].circles[j].posX, processes[i].circles[j].posY);
            j += (int) random_double(3.0, 8.0);
        }
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
