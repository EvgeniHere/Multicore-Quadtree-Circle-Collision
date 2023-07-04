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
clock_t begin;
pthread_t recvThread;

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

    //pthread_t workThread;
    //pthread_create(&workThread, NULL, workFunction, NULL);

    numProcesses = size;
    numCircles = 100000;
    circleSize = 1.0;
    maxSpeed = 1.0;
    maxCirclesPerCell = 50;
    minCellSize = 4 * circleSize;
    circle_max_X = SCREEN_WIDTH;
    circle_max_y = SCREEN_HEIGHT;

    circles = (struct Circle *) malloc(sizeof(struct Circle) * numCircles);
    processes = (struct Process*) malloc(numProcesses * sizeof(struct Process));

    if (rank == 0) {
        srand(90);
        for (int i = 0; i < numCircles; i++) {
            circles[i].id = i;
            circles[i].posX = random_double(circleSize / 2.0, SCREEN_WIDTH - circleSize / 2.0);
            circles[i].posY = random_double(circleSize / 2.0, SCREEN_HEIGHT - circleSize / 2.0);
            circles[i].velX = random_double(-maxSpeed, maxSpeed);
            circles[i].velY = random_double(-maxSpeed, maxSpeed);
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

    pthread_create(&recvThread, NULL, receiveCircle, NULL);

    if (rank == 0) {
        glutInit((int *) &argc, argv);
        begin = clock();
        initOpenGL(&argc, argv);
    } else {
        while (true) {
            update();
        }
    }

    //pthread_join(workThread, NULL);
    pthread_join(recvThread, NULL);

    MPI_Finalize();
    return 0;
}

void distributeCircles() {
    for (int i = 0; i < numProcesses; i++) {
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
    pthread_mutex_lock(&arrayMutex);

    updateTree();
    updateCirclesFromTree();

    pthread_mutex_unlock(&arrayMutex);

    if (rank != 0) {
        MPI_Send(&numCircles, 1, MPI_INT, 0, tag_numCircles, MPI_COMM_WORLD);
        MPI_Send(circles, numCircles * sizeof(struct Circle), MPI_BYTE, 0, tag_circles, MPI_COMM_WORLD);
    } else {
        processes[0].numCircles = numCircles;
        processes[0].circles = circles;
        for (int i = 1; i < numProcesses; i++) {
            MPI_Recv(&processes[i].numCircles, 1, MPI_INT, i, tag_numCircles, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            processes[i].circles = (struct Circle*) realloc(processes[i].circles, processes[i].numCircles * sizeof(struct Circle));
            MPI_Recv(processes[i].circles, processes[i].numCircles * sizeof(struct Circle), MPI_BYTE, i, tag_circles, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);

    frames++;
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    if (time_spent >= 10) {
        //printf("%d frames for 10 seconds\n", frames);
        printf("%f FPS\n", frames / 10.0);
        frames = 0;
        begin = end;
        //MPI_Finalize();
        //exit(0);
        //printTree(rootCell, 0);
    }

    if (rank == 0) {
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
    glLineWidth(3);
    glColor3f(255, 0, 0);

    for (int i = 0; i < numProcesses; i++) {
        //glLineWidth(1);
        //glColor3f(255, 255, 255);
        /*for (int j = 0; j < processes[i].numCells; j++) {
            struct Rectangle* rect = &processes[i].rects[j];
            glBegin(GL_LINE_LOOP);
            glVertex2f(rect->posX + 1, rect->posY + 1);
            glVertex2f(rect->posX + 1, rect->posY + 1 + rect->height - 2);
            glVertex2f(rect->posX + 1 + rect->width - 2, rect->posY + 1 + rect->height - 2);
            glVertex2f(rect->posX + 1 + rect->width - 2, rect->posY + 1);
            glEnd();
        }*/
        //glLineWidth(5);
        //glColor3f(255, 0, 0);
        glBegin(GL_LINE_LOOP);
        glVertex2f(processes[i].posX + 1, processes[i].posY + 1);
        glVertex2f(processes[i].posX + 1, processes[i].posY + 1 + processes[i].height - 2);
        glVertex2f(processes[i].posX + 1 + processes[i].width - 2, processes[i].posY + 1 + processes[i].height - 2);
        glVertex2f(processes[i].posX + 1 + processes[i].width - 2, processes[i].posY + 1);
        glEnd();
    }

    glColor3f(255, 255, 255);
    for (int i = 0; i < numProcesses; i++) {
        //printf("%d\n", processes[i].numCircles);
        for (int j = 0; j < processes[i].numCircles; j++) {
            drawCircle(processes[i].circles[j].posX, processes[i].circles[j].posY);
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
