#include <stdlib.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <mpi.h>
#include "window.c"
#include "quadtree.c"

double dt = 1.0;
int selectedCircle = 5742;

int main(int args, char** argv);
void update(int counter);
void display();
void mouseClick(int button, int state, int x, int y);
void drawCircle(GLfloat centerX, GLfloat centerY, GLfloat radius, int numSides);
void drawTree(struct Cell* cell, int depth);

int main(int argc, char** argv) {
    srand(90);

    numCircles = 1000;
    circleSize = 10;
    maxSpeed = circleSize / 2.0;
    maxCirclesPerCell = 3;
    minCellSize = 2 * circleSize + 4 * maxSpeed;
    circles = (struct Circle*) malloc(sizeof(struct Circle) * numCircles);

    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    setupCircles(SCREEN_WIDTH, SCREEN_HEIGHT);
    setupQuadtree(0.0, 0.0, SCREEN_WIDTH, SCREEN_HEIGHT);

    initOpenGL(&argc, argv, display, update, mouseClick);

    return 0;
}

void update(int counter) {
    checkCollisions(rootCell);
    for (int i = 0; i < numCircles; i++) {
        move(&circles[i]);
        deleteCircle(rootCell, i);
    }
    updateCell(rootCell);
    updateWindow(50.0/dt, counter);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, SCREEN_WIDTH, 0, SCREEN_HEIGHT); // Set up an orthographic projection
    glMatrixMode(GL_MODELVIEW); // WIRD DAS GEBRAUCHT???
    glLoadIdentity();
    glColor3ub(255, 255, 255);

    for (int i = 0; i < numCircles; i++) {
        GLfloat centerX = circles[i].posX;
        GLfloat centerY = circles[i].posY;
        GLfloat radius = circleSize/2.0;
        if (radius < 1.0f)
            radius = 1.0f;
        int numSides = 32;

        if (!cellContainsCircle(rootCell, i)) {
            glColor3ub(255, 0, 0);
            glPointSize(circleSize*3);
        } else {
            glColor3ub( 255, 255, 255);
            glPointSize(circleSize);
        }

        if (i == selectedCircle)
            glColor3ub(0, 255, 0);

        drawCircle(centerX, centerY, radius, numSides);
    }

    glColor3ub(255, 255, 255);

    drawTree(rootCell, 0);

    glutSwapBuffers();
}

void mouseClick(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        gravityState = !gravityState;
        printTree(rootCell, 0);
    } else if (button == 3) {
        dt += 0.1f;
        if (dt > 5.0f)
            dt = 5.0f;
    } else if (button == 4) {
        dt -= 0.1f;
        if (dt < 0.1f)
            dt = 0.1f;
    }
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

void drawTree(struct Cell* cell, int depth) {
    if (cell->isLeaf) {
        glBegin(GL_LINE_LOOP);
        glVertex2f(cell->posX + 1, cell->posY + 1); // bottom left corner
        glVertex2f(cell->posX + 1, cell->posY + 1 + cell->cellHeight - 2); // top left corner
        glVertex2f(cell->posX + 1 + cell->cellWidth - 2, cell->posY + 1 + cell->cellHeight - 2); // top right corner
        glVertex2f(cell->posX + 1 + cell->cellWidth - 2, cell->posY + 1); // bottom right corner
        glEnd();
        /*for (int i = 0; i < cell->numCirclesInCell; i++) {
            struct Circle circle = circles[cell->circle_ids[i]];
            glBegin(GL_LINES);
            glVertex2f(circle.posX, circle.posY);  // Top-left point
            glVertex2f(cell->posX + cell->cellWidth / 2, cell->posY + cell->cellHeight / 2);   // Top-right point
            glEnd();
        }*/
    } else {
        if (cell->isLeaf)
            return;
        for (int i = 0; i < 4; i++) {
            drawTree(&cell->subcells[i], depth+1);
        }
    }
}