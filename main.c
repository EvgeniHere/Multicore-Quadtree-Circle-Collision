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
#include <pthread.h>

int frames = 0;
int iterations = 0;
clock_t begin;
pthread_t recvThread;
pthread_t sendThread;

int moduloIteration;

int main(int argc, char** argv);
void update();
void display();
void drawCircle(GLfloat centerX, GLfloat centerY);
void initOpenGL(int* argc, char** argv);
void distributeCircles();

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
    circle_max_Y = SCREEN_HEIGHT;
    moduloIteration = 1;

    //startNumCircles = numCircles;
    maxOutgoing = 1000;
    circles = (struct Circle *) malloc(numCircles * sizeof(struct Circle));
    processes = (struct Process*) malloc(numProcesses * sizeof(struct Process));

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
        distributeCircles();
    } else {
        MPI_Recv(&numCircles, 1, MPI_INT, 0, tag_numCircles, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(circles, numCircles * sizeof(struct Circle), MPI_BYTE, 0, tag_circles, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    MPI_Bcast(processes, numProcesses * sizeof(struct Process), MPI_BYTE, 0, MPI_COMM_WORLD);
    setupQuadtree(processes[rank].posX, processes[rank].posY, processes[rank].width, processes[rank].height);

    if (numProcesses > 1) {
        pthread_create(&sendThread, NULL, sendCircle, NULL);
        pthread_create(&recvThread, NULL, receiveCircle, NULL);
    }

    MPI_Barrier(MPI_COMM_WORLD);

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

void distributeCircles() {
    for (int i = 0; i < numProcesses; i++) {
        processes[i].numCells = 100;
        processes[i].cells = malloc(processes[i].numCells * sizeof(struct Rectangle));
        processes[i].numCircles = 0;
        for (int j = 0; j < numCircles; j++) {
            if (!isCircleOverlappingArea(&circles[j], processes[i].posX, processes[i].posY, processes[i].width, processes[i].height))
                continue;
            processes[i].numCircles++;
        }
        processes[i].circles = malloc(processes[i].numCircles * sizeof(struct Circle));
        processes[i].numCircles = 0;
        for (int j = 0; j < numCircles; j++) {
            if (!isCircleOverlappingArea(&circles[j], processes[i].posX, processes[i].posY, processes[i].width, processes[i].height))
                continue;
            processes[i].circles[processes[i].numCircles++] = circles[j];
        }
        if (i > 0) {
            MPI_Send(&processes[i].numCircles, 1, MPI_INT, i, tag_numCircles, MPI_COMM_WORLD);
            MPI_Send(processes[i].circles, processes[i].numCircles * sizeof(struct Circle), MPI_BYTE, i, tag_circles, MPI_COMM_WORLD);
        }
    }
    numCircles = processes[0].numCircles;
    circles = processes[0].circles;
}

void update() {
    updateTree();

    /*if (iterations % 10 == 0) {
        printTree(rootCell, 0);
    }*/

    if (iterations % moduloIteration == 0) {
        updateVisualsFromTree();
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (numProcesses > 1 && iterations % moduloIteration == 0) {
        if (rank != 0) {
            MPI_Request request1, request2, request3, request4;
            MPI_Status status1, status2, status3, status4;

            MPI_Isend(&numCircles, 1, MPI_INT, 0, tag_numCircles, MPI_COMM_WORLD, &request1);
            MPI_Isend(circles, numCircles * sizeof(struct Circle), MPI_BYTE, 0, tag_circles, MPI_COMM_WORLD, &request2);
            //MPI_Isend(&numRects, 1, MPI_INT, 0, tag_numCells, MPI_COMM_WORLD, &request3);
            //MPI_Isend(rects, numRects * sizeof(struct Rectangle), MPI_BYTE, 0, tag_cells, MPI_COMM_WORLD, &request4);

            MPI_Wait(&request1, &status1);
            MPI_Wait(&request2, &status2);
            //MPI_Wait(&request3, &status3);
            //MPI_Wait(&request4, &status4);
        } else {
            MPI_Request *requests1 = malloc((numProcesses - 1) * sizeof(MPI_Request));
            MPI_Status *statuses1 = malloc((numProcesses - 1) * sizeof(MPI_Status));
            //MPI_Request *requests2 = malloc((numProcesses - 1) * sizeof(MPI_Request));
            //MPI_Status *statuses2 = malloc((numProcesses - 1) * sizeof(MPI_Status));
            for (int i = 1; i < numProcesses; i++) {
                int source;
                MPI_Request request;
                MPI_Status status;
                int num = 0;
                MPI_Irecv(&num, 1, MPI_INT, MPI_ANY_SOURCE, tag_numCircles, MPI_COMM_WORLD, &request);
                MPI_Wait(&request, &status);
                source = status.MPI_SOURCE;
                processes[source].numCircles = num;
                processes[source].circles = (struct Circle*) realloc(processes[source].circles, processes[source].numCircles * sizeof(struct Circle));
                MPI_Irecv(processes[source].circles, processes[source].numCircles * sizeof(struct Circle), MPI_BYTE, source, tag_circles, MPI_COMM_WORLD, &requests1[source - 1]);
            }

            /*for (int i = 1; i < numProcesses; i++) {
                int source;
                MPI_Request request;
                MPI_Status status;
                int num = 0;
                MPI_Irecv(&num, 1, MPI_INT, MPI_ANY_SOURCE, tag_numCells, MPI_COMM_WORLD, &request);
                MPI_Wait(&request, &status);
                source = status.MPI_SOURCE;
                processes[source].numCells = num;
                processes[source].cells = (struct Rectangle*) realloc(processes[source].cells, processes[source].numCells * sizeof(struct Rectangle));
                MPI_Irecv(processes[source].cells, processes[source].numCells * sizeof(struct Rectangle), MPI_BYTE, source, tag_cells, MPI_COMM_WORLD, &requests2[source - 1]);
            }*/
            MPI_Waitall(numProcesses - 1, requests1, statuses1);
            free(requests1);
            free(statuses1);
            /*MPI_Waitall(numProcesses - 1, requests2, statuses2);
            free(requests2);
            free(statuses2);*/
        }
    }

    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    if (time_spent >= 10) {
        //printf("%d frames for 10 seconds\n", frames);
        if (rank == 0)
            printf("%f FPS\n", frames / time_spent);
        frames = 0;
        iterations = 0;
        begin = end;
        //MPI_Finalize();
        //exit(0);
        //printTree(rootCell, 0);
    }

    iterations++;
    if (rank == 0) {
        if ((iterations-1) % moduloIteration == 0) {
            processes[0].numCells = numRects;
            processes[0].cells = rects;
            processes[0].numCircles = numCircles;
            processes[0].circles = circles;
            glutPostRedisplay();
            frames++;
        }
        glutTimerFunc(10, update, 0);
        //printTree(rootCell, 0);
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
    //glLineWidth(3);
    //glColor3f(255, 0, 0);

    /*for (int i = 0; i < numProcesses; i++) {
        glLineWidth(1);
        glColor3f(255, 255, 255);
        for (int j = 0; j < processes[i].numCells; j++) {
            struct Rectangle* rect = &processes[i].cells[j];
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
        //printf("%d\n", processes[i].numCircles);
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
