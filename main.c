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

int frames = 0;
clock_t begin;

pthread_t recvThread;
pthread_t sendThread;

int numProcesses;

int main(int argc, char** argv);
void update();
void display();
void drawCircle(GLfloat centerX, GLfloat centerY);
void initOpenGL(int* argc, char** argv);
void distributeCircles();

struct Process* processes;
struct Process* curProcess;
struct Circle *sendCircles;

int main(int argc, char** argv) {
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    if (provided != MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI does not provide the required thread support.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

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
    gravity = 0.000001;
    circle_max_X = SCREEN_WIDTH;
    circle_max_y = SCREEN_HEIGHT;
    maxOutgoing = 1000;
    maxIngoing = 1000;

    circles = (struct Circle *) malloc(sizeof(struct Circle) * numCircles);
    curProcess = (struct Process*) malloc(sizeof(struct Process));
    processes = (struct Process*) malloc(numProcesses * sizeof(struct Process));
    sendCircles = (struct Circle *) malloc(1 * sizeof(struct Circle));

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
        }
        curProcess = &processes[0];
    } else {
        MPI_Recv(curProcess, sizeof(struct Process), MPI_BYTE, 0, tag_process, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    MPI_Bcast(processes, numProcesses * sizeof(struct Process), MPI_BYTE, 0, MPI_COMM_WORLD);
    MPI_Bcast(circles, numCircles * sizeof(struct Circle), MPI_BYTE, 0, MPI_COMM_WORLD);
    setupQuadtree(curProcess->posX, curProcess->posY, curProcess->width, curProcess->height);

    if (numProcesses > 1) {
        pthread_create(&sendThread, NULL, sendCircle, NULL);
        pthread_create(&recvThread, NULL, receiveCircle, NULL);
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

    pthread_join(recvThread, NULL);
    pthread_join(sendThread, NULL);

    MPI_Finalize();
    return 0;
}

void update() {
    updateTree();

    if (frames % 4 == 0) {
        int index = 0;
        sendCircles = (struct Circle *) realloc(sendCircles, numCirclesInside * sizeof(struct Circle));
        for (int i = 0; i < numCircles; i++) {
            if (!circle_inside[i])
                continue;
            sendCircles[index++] = circles[i];
        }
        if (rank == 0) {
            processes[0].numCircles = numCirclesInside;
            processes[0].circles = sendCircles;
        }
        if (size > 1) {
            if (rank != 0) {
                MPI_Request request1, request2;
                MPI_Status status1, status2;
                MPI_Isend(&numCirclesInside, 1, MPI_INT, 0, tag_numCircles, MPI_COMM_WORLD, &request1);
                MPI_Isend(sendCircles, numCirclesInside * sizeof(struct Circle), MPI_BYTE, 0, tag_circles,
                          MPI_COMM_WORLD, &request2);
                MPI_Wait(&request1, &status1);
                MPI_Wait(&request2, &status2);
            } else {
                MPI_Request* requests = (MPI_Request*) malloc((numProcesses-1) * sizeof(MPI_Request));
                MPI_Status* statuses = (MPI_Status*) malloc((numProcesses-1) * sizeof(MPI_Status));
                for (int i = 1; i < numProcesses; i++) {
                    MPI_Request request1;
                    MPI_Status status1;
                    int num = 0;
                    MPI_Irecv(&num, 1, MPI_INT, MPI_ANY_SOURCE, tag_numCircles, MPI_COMM_WORLD, &request1);
                    MPI_Wait(&request1, &status1);
                    int source = status1.MPI_SOURCE;
                    processes[source].numCircles = num;
                    processes[source].circles = (struct Circle*) realloc(processes[source].circles, processes[source].numCircles * sizeof(struct Circle));
                    MPI_Irecv(processes[source].circles, processes[source].numCircles * sizeof(struct Circle), MPI_BYTE, source,
                              tag_circles, MPI_COMM_WORLD, &requests[source-1]);
                }
                MPI_Waitall(numProcesses-1, requests, statuses);
                free(requests);
                free(statuses);
            }
        }
    }

    frames++; //Besser vorher -> verschobene leistung der prozesse
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
        if (frames % 4 == 0) {
            glutPostRedisplay();
        }
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
            j += (int) random_double(1.0, 8.0);
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
