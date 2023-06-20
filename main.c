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

int frames = 0;
clock_t begin;

bool* circleIsInBL;
bool* circleIsInBR;
bool* circleIsInTR;
bool* circleIsInTL;

int rank, size;

int main(int args, char** argv);
void update();
void display();
void drawCircle(GLfloat centerX, GLfloat centerY, GLfloat radius, int numSides);
void initOpenGL(int* argc, char** argv);
int sleep(long ms);

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    printf("%d %d\n", rank, size);

    if (size < 5) {
        fprintf(stderr, "This program requires at least 5 processes.\n");
        MPI_Finalize();
        return 1;
    }

    numCircles = 1000;
    circleSize = 10.0;
    maxSpeed = circleSize / 4.0;
    maxCirclesPerCell = 3;
    minCellSize = 2 * circleSize + 4 * maxSpeed;
    circle_max_X = SCREEN_WIDTH;
    circle_max_y = SCREEN_HEIGHT;

    circles = (struct Circle*) malloc(sizeof(struct Circle) * numCircles);

    if (rank == 0) {
        srand(90);
        circleIsInBL = (bool*) malloc(numCircles * sizeof(bool));
        circleIsInBR = (bool*) malloc(numCircles * sizeof(bool));
        circleIsInTR = (bool*) malloc(numCircles * sizeof(bool));
        circleIsInTL = (bool*) malloc(numCircles * sizeof(bool));

        for (int i = 0; i < numCircles; i++) {
            circles[i].posX = random_double(circleSize/2.0, SCREEN_WIDTH - circleSize/2.0);
            circles[i].posY = random_double(circleSize/2.0, SCREEN_HEIGHT - circleSize/2.0);
            circles[i].velX = random_double(-maxSpeed, maxSpeed);
            circles[i].velY = random_double(-maxSpeed, maxSpeed);
            circleIsInBL[i] = isCircleOverlappingArea(&circles[i], 0, 0, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
            circleIsInBR[i] = isCircleOverlappingArea(&circles[i], SCREEN_WIDTH / 2, 0, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
            circleIsInTR[i] = isCircleOverlappingArea(&circles[i], SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
            circleIsInTL[i] = isCircleOverlappingArea(&circles[i], 0, SCREEN_HEIGHT / 2, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
        }
    }

    MPI_Bcast(circles, numCircles * sizeof(struct Circle), MPI_BYTE, 0, MPI_COMM_WORLD);

    if (rank == 1) {
        setupQuadtree(0, 0, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    } else if (rank == 2) {
        setupQuadtree(SCREEN_WIDTH / 2, 0, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    } else if (rank == 3) {
        setupQuadtree(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    } else if (rank == 4) {
        setupQuadtree(0, SCREEN_HEIGHT / 2, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    }

    glutInit((int *) &argc, argv);
    if (rank == 0) {
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
        checkCollisions(rootCell);
        for (int i = 0; i < numCircles; i++) {
            move(&circles[i]);
            deleteCircle(rootCell, i);
        }
        updateCell(rootCell);
        MPI_Send(circles, numCircles * sizeof(struct Circle), MPI_BYTE, 0, tag_circles, MPI_COMM_WORLD);
    } else {
        struct Circle *recv_circles = (struct Circle *) malloc(numCircles * sizeof(struct Circle));
        MPI_Recv(recv_circles, numCircles * sizeof(struct Circle), MPI_BYTE, 1, tag_circles, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
        for (int i = 0; i < numCircles; i++) {
            if (circleIsInBL[i])
                circles[i] = *circleCopy(&recv_circles[i]);
            circleIsInBL[i] = isCircleOverlappingArea(&circles[i], 0, 0, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
        }
        MPI_Recv(recv_circles, numCircles * sizeof(struct Circle), MPI_BYTE, 2, tag_circles, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
        for (int i = 0; i < numCircles; i++) {
            if (circleIsInBR[i])
                circles[i] = *circleCopy(&recv_circles[i]);
            circleIsInBR[i] = isCircleOverlappingArea(&circles[i], SCREEN_WIDTH / 2, 0, SCREEN_WIDTH / 2,
                                                      SCREEN_HEIGHT / 2);
        }
        MPI_Recv(recv_circles, numCircles * sizeof(struct Circle), MPI_BYTE, 3, tag_circles, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
        for (int i = 0; i < numCircles; i++) {
            if (circleIsInTR[i])
                circles[i] = *circleCopy(&recv_circles[i]);
            circleIsInTR[i] = isCircleOverlappingArea(&circles[i], SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2,
                                                      SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
        }
        MPI_Recv(recv_circles, numCircles * sizeof(struct Circle), MPI_BYTE, 4, tag_circles, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
        for (int i = 0; i < numCircles; i++) {
            if (circleIsInTL[i])
                circles[i] = *circleCopy(&recv_circles[i]);
            circleIsInTL[i] = isCircleOverlappingArea(&circles[i], 0, SCREEN_HEIGHT / 2, SCREEN_WIDTH / 2,
                                                      SCREEN_HEIGHT / 2);
        }
        free(recv_circles);
    }

    MPI_Bcast(circles, numCircles * sizeof(struct Circle), MPI_BYTE, 0, MPI_COMM_WORLD);

    for (int i = 0; i < numCircles; i++) {
        if (rank == 1) {
            if (isCircleOverlappingArea(&circles[i], 0, 0, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2))
                addCircleToCell(i, rootCell);
        } else if (rank == 2) {
            if (isCircleOverlappingArea(&circles[i], SCREEN_WIDTH / 2, 0, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2))
                addCircleToCell(i, rootCell);
        } else if (rank == 3) {
            if (isCircleOverlappingArea(&circles[i], SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, SCREEN_WIDTH / 2,
                                        SCREEN_HEIGHT / 2))
                addCircleToCell(i, rootCell);
        } else if (rank == 4) {
            if (isCircleOverlappingArea(&circles[i], 0, SCREEN_HEIGHT / 2, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2))
                addCircleToCell(i, rootCell);
        }
    }

    if (rank == 0) {
        frames++;
        if (frames >= 1000) {
            clock_t end = clock();
            double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
            printf("%f seconds for 1000 frames\n", time_spent);
            frames = 0;
            begin = end;
        }
        glutPostRedisplay();
        glutTimerFunc(0, update, 0);
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, SCREEN_WIDTH, 0, SCREEN_HEIGHT); // Set up an orthographic projection
    glMatrixMode(GL_MODELVIEW); // WIRD DAS GEBRAUCHT???
    glLoadIdentity();
    glPointSize(circleSize);
    glColor3ub(255, 255, 255);

    for (int i = 0; i < numCircles; i++) {
        GLfloat centerX = circles[i].posX;
        GLfloat centerY = circles[i].posY;
        GLfloat radius = circleSize/2.0;
        if (radius < 1.0f)
            radius = 1.0f;
        int numSides = 32;

        drawCircle(centerX, centerY, radius, numSides);
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
    glutCreateWindow("Bouncing Circles");
    glutCloseFunc(closeWindow);
    glutDisplayFunc(display);
    glutTimerFunc(0, update, 0);
    glutMainLoop();
}

void drawCircle(GLfloat centerX, GLfloat centerY, GLfloat radius, int numSides) {
    if (circleSize <= 2.0f) {
        glBegin(GL_POINTS);
        glVertex2i(centerX, centerY);
        glEnd();
    } else {
        GLfloat angleIncrement = 2.0 * M_PI / numSides;
        glBegin(GL_POLYGON);
        for (int i = 0; i < numSides; i++) {
            GLfloat angle = i * angleIncrement;
            GLfloat x = centerX + radius * cos(angle);
            GLfloat y = centerY + radius * sin(angle);
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
