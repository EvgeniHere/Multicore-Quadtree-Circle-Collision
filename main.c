#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
//#include <GL/glut.h>
//#include <GL/gl.h>
//#include <omp.h>

#define SCREEN_WIDTH 1000.0
#define SCREEN_HEIGHT 1000.0
#define numCircles 6000
#define circleSize 5
#define maxCirclesPerCell 3
#define maxSpawnSpeed (circleSize / 4.0)
#define maxSpeed (circleSize / 4.0)
#define gravity 0.01


bool gravityState = true; //Mouseclick ins Fenster
bool drawCells = true; //Zeichnet tiefste Zellen des Baums
double dt = 1.0;
int selectedCircle = 5742;
double friction = 0.9;
double minCellSize = 2 * circleSize + 4 * maxSpeed;
clock_t begin;
int frames = 0;


struct Circle {
    float posX;
    float posY;
    float velX;
    float velY;
    int isUsed;
};

struct Circle circles[numCircles];

struct Cell {
    float posX;
    float posY;
    float cellWidth;
    float cellHeight;
    int numCirclesInCell;
    bool isLeaf;
    int* circle_ids;
    struct Cell* subCells;
};

struct Cell* rootCell;

// Define the number of sides of the polygon used to approximate the circle

void move(int circle_id);
void addCircleToCell(int circle_id, struct Cell* cell, bool checkCollision);
void deleteTree(struct Cell* cell);
bool isCircleInCellArea(int circle_id, struct Cell cell);
void split(struct Cell* cell);
float random_float(float min, float max);
void checkCollisions(int circle_id, struct Cell* cell);
/*
void mouseClick(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        gravityState = (gravityState + 1) % 2;
    }
}

void drawCircle(GLfloat centerX, GLfloat centerY, GLfloat radius, int numSides) {
    GLfloat angleIncrement = 2.0 * M_PI / numSides;
    if (circleSize <= 2.0f) {
        glPointSize(circleSize);
        glBegin(GL_POINTS);
        glVertex2i(centerX, centerY); //Set pixel coordinates
        glEnd();
    } else {
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

void drawTree(struct Cell* cell) {
    if (cell->isLeaf) {
        glBegin(GL_LINE_LOOP);
        glVertex2f(cell->posX, cell->posY); // bottom left corner
        glVertex2f(cell->posX, cell->posY + cell->cellHeight); // top left corner
        glVertex2f(cell->posX + cell->cellWidth, cell->posY + cell->cellHeight); // top right corner
        glVertex2f(cell->posX + cell->cellWidth, cell->posY); // bottom right corner
        glEnd();
    } else {
        for (int i = 0; i < 4; i++) {
            drawTree(&cell->subCells[i]);
        }
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, SCREEN_WIDTH, 0, SCREEN_HEIGHT); // Set up an orthographic projection
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Draw three circles at different positions
    for (int i = 0; i < numCircles; i++) {
        GLfloat centerX = circles[i].posX;
        GLfloat centerY = circles[i].posY;
        GLfloat radius = circleSize/2;
        if (radius < 1.0f)
            radius = 1.0f;
        int numSides = 32;
        drawCircle(centerX, centerY, radius, numSides);
    }

    drawTree(rootCell);

    glutSwapBuffers();
}
*/
void update(int counter) {
    deleteTree(rootCell);
    rootCell->circle_ids = (int*)malloc(maxCirclesPerCell * sizeof(int));

    for (int i = 0; i < numCircles; i++) {
        addCircleToCell(i, rootCell, true);
        move(i);
    }
    frames++;
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    if (time_spent >= 10) {
        printf("%f fps\n", frames/time_spent);
        frames = 0;
        begin = end;
    }

    //glutPostRedisplay();
    //glutTimerFunc(16, update, counter + 1);

    //if (counter % saveIntervall == 0)
        //save_Iteration(file);
}

int main(int argc, char** argv) {

    srand(90);

    rootCell = (struct Cell*)malloc(sizeof(struct Cell));
    rootCell->posX = 0.0f;
    rootCell->posY = 0.0f;
    rootCell->cellWidth = (float) SCREEN_WIDTH;
    rootCell->cellHeight = (float) SCREEN_HEIGHT;
    rootCell->isLeaf = true;
    rootCell->numCirclesInCell = 0;
    //rootCell->circle_ids = (int*)malloc(maxCirclesPerCell * sizeof(int));

    for (int i = 0; i < numCircles; i++) {
        circles[i].posX = random_float(circleSize/2, SCREEN_WIDTH-circleSize/2);
        circles[i].posY = random_float(circleSize/2, SCREEN_HEIGHT-circleSize/2);
        circles[i].velX = random_float(-maxSpawnSpeed, maxSpawnSpeed);
        circles[i].velY = random_float(-maxSpawnSpeed, maxSpawnSpeed);
        circles[i].isUsed = 0;
    }
    /*
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutInitWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
    glutCreateWindow("Bouncing Circles");
    glutDisplayFunc(display);
    glutTimerFunc(16, update, 0);
    glutMouseFunc(mouseClick);
    glutMainLoop();*/

    clock_t begin = clock();

    while(true) {
        update(0);
    }

    return 0;
}

void move(int circle_id) {
    struct Circle* circle = &circles[circle_id];
    if (gravityState)
        circle->velY -= gravity;

    if (circle->velX > maxSpeed)
        circle->velX = maxSpeed;
    if (circle->velY > maxSpeed)
        circle->velY = maxSpeed;
    if (circle->velX < -maxSpeed)
        circle->velX = -maxSpeed;
    if (circle->velY < -maxSpeed)
        circle->velY = -maxSpeed;

    double stepDistX = circle->velX;
    double stepDistY = circle->velY;

    if (circle->posX + circleSize/2.0 + stepDistX > SCREEN_WIDTH && circle->velX > 0.0) {
        circle->posX = SCREEN_WIDTH - circleSize/2.0 - stepDistX;
        circle->velX *= -friction;
    } else if (circle->posX - circleSize/2.0 + stepDistX < 0.0 && circle->velX < 0.0) {
        circle->posX = circleSize/2.0 - stepDistX;
        circle->velX *= -friction;
    }

    if (circle->posY + circleSize/2.0 + stepDistY > SCREEN_HEIGHT && circle->velY > 0.0) {
        circle->posY = SCREEN_HEIGHT - circleSize/2.0 - stepDistY;
        circle->velY *= -friction;
    } else if (circle->posY - circleSize/2.0 + stepDistY < 0.0 && circle->velY < 0.0) {
        circle->posY = circleSize/2.0 - stepDistY;
        circle->velY *= -friction;
    }

    circle->posX += circle->velX;
    circle->posY += circle->velY;
}

void checkPosition(struct Circle* circle) {
    if (circle->posX + circleSize/2.0 > SCREEN_WIDTH) {
        circle->posX = SCREEN_WIDTH - circleSize/2.0;
    } else if (circle->posX - circleSize/2.0 < 0.0) {
        circle->posX = circleSize/2.0;
    }

    if (circle->posY + circleSize/2.0 > SCREEN_HEIGHT) {
        circle->posY = SCREEN_HEIGHT - circleSize/2.0;
    } else if (circle->posY - circleSize/2.0 < 0.0) {
        circle->posY = circleSize/2.0;
    }
}


void checkCollisions(int circle_id, struct Cell* cell) {
    int id_1 = circle_id;
    for (int i = 0; i < cell->numCirclesInCell; i++) {
        int id_2 = i;
        if (id_1 == id_2)
            continue;
        if (fabs(circles[id_1].posX - circles[id_2].posX) > circleSize ||
            fabs(circles[id_1].posY - circles[id_2].posY) > circleSize)
            continue;

        double dx = circles[id_2].posX - circles[id_1].posX;
        double dy = circles[id_2].posY - circles[id_1].posY;
        double distSquared = dx * dx + dy * dy;
        double sum_r = circleSize;

        if (distSquared < sum_r * sum_r) {
            if (distSquared < sum_r * sum_r) {
                double dist = sqrt(distSquared);
                double overlap = (sum_r - dist) / 2.0;
                dx /= dist;
                dy /= dist;

                circles[id_1].posX -= overlap * dx;
                circles[id_1].posY -= overlap * dy;
                circles[id_2].posX += overlap * dx;
                circles[id_2].posY += overlap * dy;

                double dvx = circles[id_2].velX - circles[id_1].velX;
                double dvy = circles[id_2].velY - circles[id_1].velY;
                double dot = dvx * dx + dvy * dy;

                circles[id_1].velX += dot * dx;
                circles[id_1].velY += dot * dy;
                circles[id_2].velX -= dot * dx;
                circles[id_2].velY -= dot * dy;

                circles[id_1].velX *= friction;
                circles[id_1].velY *= friction;
                circles[id_2].velX *= friction;
                circles[id_2].velY *= friction;
            }
        }
    }

    for (int i = 0; i < cell->numCirclesInCell; i++) {
        int id = cell->circle_ids[i];
        checkPosition(&circles[id]);
    }
}

void deleteTree(struct Cell* cell) {
    if (!cell->isLeaf) {
        for (int i = 0; i < 4; i++) {
            deleteTree(&cell->subCells[i]);
        }
        free(cell->subCells);
        cell->isLeaf = true;
    } else {
        if (cell->numCirclesInCell != 0) {
            free(cell->circle_ids);
            cell->numCirclesInCell = 0;
        }
    }
}

bool isCircleInCellArea(int circle_id, struct Cell cell) {
    return circles[circle_id].posX + circleSize / 2 > cell.posX && circles[circle_id].posX - circleSize / 2 < cell.posX + cell.cellWidth && circles[circle_id].posY + circleSize / 2 > cell.posY && circles[circle_id].posY - circleSize / 2 < cell.posY + cell.cellHeight;
}

void split(struct Cell* cell) {
    cell->subCells = (struct Cell*)malloc(4 * sizeof(struct Cell));
    if(cell->subCells == NULL) {
        printf("Memory error!");
        exit(1);
    }

    for (int i = 0; i < 4; i++) {
        cell->subCells[i].cellWidth = cell->cellWidth / 2;
        cell->subCells[i].cellHeight = cell->cellHeight / 2;
        cell->subCells[i].isLeaf = true;
        cell->subCells[i].numCirclesInCell = 0;
        cell->subCells[i].circle_ids = (int*)malloc(maxCirclesPerCell * sizeof(int));
    }

    cell->subCells[0].posX = cell->posX;
    cell->subCells[0].posY = cell->posY;
    cell->subCells[1].posX = cell->posX + cell->cellWidth / 2;
    cell->subCells[1].posY = cell->posY;
    cell->subCells[2].posX = cell->posX + cell->cellWidth / 2;
    cell->subCells[2].posY = cell->posY + cell->cellHeight / 2;
    cell->subCells[3].posX = cell->posX;
    cell->subCells[3].posY = cell->posY + cell->cellHeight / 2;

    cell->isLeaf = false;
}

void addCircleToCell(int circle_id, struct Cell* cell, bool checkCollision) {
    if (!cell->isLeaf) {
        for (int i = 0; i < 4; i++)
            if (isCircleInCellArea(circle_id, cell->subCells[i]))
                addCircleToCell(circle_id, &cell->subCells[i], checkCollision);
        return;
    }

    if (cell->numCirclesInCell < maxCirclesPerCell || cell->cellWidth < 5*circleSize || cell->cellHeight < 5*circleSize) {
        if(checkCollision)
            checkCollisions(circle_id, cell);
        if (cell->numCirclesInCell >= maxCirclesPerCell)
            cell->circle_ids = (int*)realloc(cell->circle_ids, (cell->numCirclesInCell + 1) * sizeof(int));
        cell->circle_ids[cell->numCirclesInCell] = circle_id;
        cell->numCirclesInCell++;
        return;
    }

    split(cell);

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < cell->numCirclesInCell; j++) {
            if (isCircleInCellArea(cell->circle_ids[j], cell->subCells[i])) {
                addCircleToCell(cell->circle_ids[j], &cell->subCells[i], false);
            }
        }

        if (isCircleInCellArea(circle_id, cell->subCells[i]))
            addCircleToCell(circle_id, &cell->subCells[i], true);
    }

    cell->numCirclesInCell = 0;
    free(cell->circle_ids);
}

float random_float(float min, float max) {
    return (max - min) * ( (float)rand() / (float)RAND_MAX ) + min;
}
