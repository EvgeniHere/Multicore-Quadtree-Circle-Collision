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

#define SCREEN_WIDTH 1000.0
#define SCREEN_HEIGHT 1000.0

int tag_circles = 1;
int tag_numCircles = 2;

int frames = 0;
clock_t begin;

int rank, size;

int rows;
int cols;
int numProcesses;

int main(int argc, char** argv);
void update();
void display();
void drawCircle(GLfloat centerX, GLfloat centerY, GLfloat radius, int numSides);
void initOpenGL(int* argc, char** argv);
void distributeCircles();
int sleep(long ms);

struct Process {
    int numCircles;
    int* circle_ids;
    struct Circle* circles;
};

struct Process* processes;

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

    numProcesses = size - 1;
    processes = (struct Process*) malloc(numProcesses * sizeof(struct Process));
    cols = sqrt(numProcesses);
    rows = (numProcesses + cols - 1) / cols;

    numCircles = 1000;
    circleSize = 10.0;
    maxSpeed = circleSize / 4.0;
    maxCirclesPerCell = 3;
    minCellSize = 2 * circleSize + 4 * maxSpeed;
    circle_max_X = SCREEN_WIDTH;
    circle_max_y = SCREEN_HEIGHT;

    if (rank == 0) {
        srand(90);
        circles = (struct Circle *) malloc(sizeof(struct Circle) * numCircles);
    }

    distributeCircles();

    if (rank != 0) {
        int selectedRow = (rank - 1) / cols;
        int selectedCol = (rank - 1) % cols;
        int posX = selectedCol * SCREEN_WIDTH / cols;
        int posY = selectedRow * SCREEN_HEIGHT / rows;
        setupQuadtree(posX, posY, SCREEN_WIDTH / cols, SCREEN_HEIGHT / rows);
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
        for (int i = 0; i < numCircles; i++) {
            move(&circles[i]);
            deleteCircle(rootCell, i);
        }
        checkCollisions(rootCell);
        updateCell(rootCell);
        MPI_Send(circles, numCircles * sizeof(struct Circle), MPI_BYTE, 0, tag_circles, MPI_COMM_WORLD);
    } else {
        for (int i = 0; i < numProcesses; i++) {
            MPI_Recv(processes[i].circles, processes[i].numCircles * sizeof(struct Circle), MPI_BYTE, i + 1, tag_circles, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            for (int j = 0; j < processes[i].numCircles; j++) {
                circles[processes[i].circle_ids[j]] = processes[i].circles[j];
            }
        }
    }

    distributeCircles();

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

void distributeCircles() {
    if (rank == 0) {
        for (int i = 0; i < numProcesses; i++) {
            processes[i].numCircles = 0;
        }

        for (int i = 0; i < numCircles; i++) {
            int colIndex = (int) (circles[i].posX / (float) SCREEN_WIDTH * cols);
            int rowIndex = (int) (circles[i].posY / (float) SCREEN_HEIGHT * rows);
            int process_index = rowIndex * cols + colIndex;
            processes[process_index].numCircles++;
        }

        for (int i = 0; i < numProcesses; i++) {
            processes[i].circles = (struct Circle *) realloc(processes[i].circles,processes[i].numCircles * sizeof(struct Circle));
            processes[i].circle_ids = (int *) realloc(processes[i].circle_ids, processes[i].numCircles * sizeof(int));
            processes[i].numCircles = 0;
        }

        for (int i = 0; i < numCircles; i++) {
            int colIndex = (int) (circles[i].posX / (float) SCREEN_WIDTH * cols);
            int rowIndex = (int) (circles[i].posY / (float) SCREEN_HEIGHT * rows);
            int process_index = rowIndex * cols + colIndex;
            processes[process_index].circles[processes[process_index].numCircles] = circles[i];
            processes[process_index].circle_ids[processes[process_index].numCircles] = i;
            processes[process_index].numCircles++;
        }

        for (int i = 0; i < numProcesses; i++) {
            MPI_Send(&processes[i].numCircles, sizeof(int), MPI_INT, i + 1, tag_numCircles, MPI_COMM_WORLD);
            MPI_Send(processes[i].circles, processes[i].numCircles * sizeof(struct Circle), MPI_BYTE, i + 1, tag_numCircles, MPI_COMM_WORLD);
        }
    } else {
        MPI_Recv(&numCircles, sizeof(int), MPI_INT, 0, tag_numCircles, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        circles = (struct Circle*) realloc(circles, sizeof(struct Circle) * numCircles);
        MPI_Recv(circles, numCircles * sizeof(struct Circle), MPI_BYTE, 0, tag_numCircles, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
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
