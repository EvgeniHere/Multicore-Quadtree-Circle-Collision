#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <GL/glut.h>
#include <GL/gl.h>

#define SCREEN_WIDTH 1000.0f
#define SCREEN_HEIGHT 1000.0f
#define NUM_CIRCLES 11
#define CIRCLE_SIZE 40.0f
#define MAX_CIRCLES_PER_CELL 2
#define MAX_SPAWN_SPEED 8.0f
#define MAX_SPEED 8.0f
#define GRAVITY_FORCE 0.1f

bool gravityState = true; //Mouseclick ins Fenster
bool drawCells = true; //Zeichnet tiefste Zellen des Baums
float dt = 2.0f;
int selectedCircle = 6;

struct Circle {
    float posX;
    float posY;
    float velX;
    float velY;
    int isUsed;
    /*int red;
    int green;
    int blue;*/
};

struct Circle circles[NUM_CIRCLES];

struct Cell {
    float posX;
    float posY;
    float cellWidth;
    float cellHeight;
    int numCirclesInCell;
    bool isLeaf;
    int* circle_ids;
    struct Cell* subcells;
    struct Cell* parentCell;
    bool selected;
};

struct Cell* rootCell;

void move(int circle_id);
void addCircleToCell(int circle_id, struct Cell* cell);
bool isCircleOverCellArea(int circle_id, struct Cell* cell);
void split(struct Cell* cell);
float random_float(float min, float max);
void checkCollisions(struct Cell* cell);
void updateCell(struct Cell* cell);
void printTree(struct Cell* cell, int depth);
void addCircleToParentCell(int circle_id, struct Cell* cell);

void mouseClick(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        gravityState = !gravityState;
        printTree(rootCell, 0);
        /*printf("%d; %d\n", x, y);
        for (int i = 0; i < NUM_CIRCLES; i++) {
            if (x > (int)circles[i].posX - CIRCLE_SIZE / 2 && x < (int)circles[i].posX + CIRCLE_SIZE / 2 &&
                y > (int)circles[i].posY - CIRCLE_SIZE / 2 && y < (int)circles[i].posY + CIRCLE_SIZE / 2) {
                selectedCircle = i;
            }
        }*/
    } else if (button == 3) {
        dt += 0.1f;
        //printf("%f\n", 10.0f/dt);
        //printf("%f\n", dt);
    } else if (button == 4) {
        dt -= 0.1f;
        if (dt < 0.1f)
            dt = 0.1f;
        //printf("%f\n", 10.0f/dt);
        //printf("%f\n", dt);
    }
}

void drawCircle(GLfloat centerX, GLfloat centerY, GLfloat radius, int numSides) {
    GLfloat angleIncrement = 2.0 * M_PI / numSides;
    if (CIRCLE_SIZE <= 2.0f) {
        glPointSize(CIRCLE_SIZE);
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

void drawTree(struct Cell* cell, int depth) {
    if (!drawCells)
        return;
    if (cell->isLeaf) {
        if (cell->selected)
            glColor3ub(255, 0, 0);
        else
            glColor3ub( 255, 255, 255 );
        glBegin(GL_LINE_LOOP);
        glVertex2f(cell->posX + 1, cell->posY + 1); // bottom left corner
        glVertex2f(cell->posX + 1, cell->posY + 1 + cell->cellHeight - 2); // top left corner
        glVertex2f(cell->posX + 1 + cell->cellWidth - 2, cell->posY + 1 + cell->cellHeight - 2); // top right corner
        glVertex2f(cell->posX + 1 + cell->cellWidth - 2, cell->posY + 1); // bottom right corner
        glEnd();
    } else {
    if (cell->isLeaf)
        return;
        for (int i = 0; i < 4; i++) {
            drawTree(&cell->subcells[i], depth+1);
        }
    }
}

void printTree(struct Cell* cell, int depth) {
    for (int i = 0; i < depth; i++) {
        printf("----");
    }
    printf("%d; %d\n", cell->numCirclesInCell, cell->isLeaf);

    if (cell->isLeaf)
        return;

    for (int i = 0; i < 4; i++)
        printTree(&cell->subcells[i], depth+1);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, SCREEN_WIDTH, 0, SCREEN_HEIGHT); // Set up an orthographic projection
    glMatrixMode(GL_MODELVIEW); // WIRD DAS GEBRAUCHT???
    glLoadIdentity();
    glColor3ub( 255, 255, 255 );

    for (int i = 0; i < NUM_CIRCLES; i++) {
        GLfloat centerX = circles[i].posX;
        GLfloat centerY = circles[i].posY;
        GLfloat radius = CIRCLE_SIZE / 2;
        if (radius < 1.0f)
            radius = 1.0f;
        int numSides = 32;
        //glColor3ub(circles[i].red, circles[i].green, circles[i].blue);

        if (i == selectedCircle)
            glColor3ub(255, 0, 0);
        drawCircle(centerX, centerY, radius, numSides);

        glColor3ub( 255, 255, 255 );
    }

    glColor3ub(255, 255, 255);

    drawTree(rootCell, 0);

    glutSwapBuffers();
}

void update(int counter) {
    //printTree(rootCell, 0);
    //printf("\n--UPDATE--\n");
    for (int i = 0; i < NUM_CIRCLES; i++) {
        move(i);
    }
    updateCell(rootCell);
    glutPostRedisplay();

    glutTimerFunc(50.0f/dt, update, counter + 1);
}

int main(int argc, char** argv) {
    srand(90);

    rootCell = (struct Cell*)malloc(sizeof(struct Cell));
    if (rootCell == NULL) {
        printf("Memory error!");
        exit(1);
    }
    rootCell->posX = 0.0f;
    rootCell->posY = 0.0f;
    rootCell->cellWidth = (float) SCREEN_WIDTH;
    rootCell->cellHeight = (float) SCREEN_HEIGHT;
    rootCell->isLeaf = true;
    rootCell->numCirclesInCell = 0;
    rootCell->parentCell = NULL;
    rootCell->circle_ids = (int*)malloc(MAX_CIRCLES_PER_CELL * sizeof(int));
    if (rootCell->circle_ids == NULL) {
        printf("Memory error!");
        exit(1);
    }
    rootCell->subcells = NULL;
    for (int i = 0; i < NUM_CIRCLES; i++) {
        circles[i].posX = random_float(CIRCLE_SIZE / 2, SCREEN_WIDTH - CIRCLE_SIZE / 2);
        circles[i].posY = random_float(CIRCLE_SIZE / 2, SCREEN_HEIGHT - CIRCLE_SIZE / 2);
        circles[i].velX = random_float(-MAX_SPAWN_SPEED, MAX_SPAWN_SPEED);
        circles[i].velY = random_float(-MAX_SPAWN_SPEED, MAX_SPAWN_SPEED);
        circles[i].isUsed = 0;
    }

    for (int i = 0; i < NUM_CIRCLES; i++) {
        addCircleToCell(i, rootCell);
        updateCell(rootCell);
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutInitWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
    glutCreateWindow("Bouncing Circles");
    glutDisplayFunc(display);
    glutTimerFunc(16, update, 0);
    glutMouseFunc(mouseClick);
    glutMainLoop();

    return 0;
}

void move(int circle_id) {
    struct Circle* circle = &circles[circle_id];
    if (gravityState)
        circle->velY -= GRAVITY_FORCE;

    if (circle->velX > MAX_SPEED)
        circle->velX = MAX_SPEED;
    if (circle->velY > MAX_SPEED)
        circle->velY = MAX_SPEED;
    if (circle->velX < -MAX_SPEED)
        circle->velX = -MAX_SPEED;
    if (circle->velY < -MAX_SPEED)
        circle->velY = -MAX_SPEED;

    float stepDistX = circle->velX * dt;
    float stepDistY = circle->velY * dt;

    if (circle->posX + CIRCLE_SIZE / 2 > SCREEN_WIDTH - stepDistX && circle->velX > 0.0f) {
        circle->posX = SCREEN_WIDTH - CIRCLE_SIZE / 2 - stepDistX;
        circle->velX *= -1;
    } else if (circle->posX - CIRCLE_SIZE / 2 < -stepDistX && circle->velX < 0.0f) {
        circle->posX = CIRCLE_SIZE / 2 - stepDistX;
        circle->velX *= -1;
    }

    if (circle->posY + CIRCLE_SIZE / 2 > SCREEN_HEIGHT - stepDistY && circle->velY > 0.0f) {
        circle->posY = SCREEN_HEIGHT - CIRCLE_SIZE / 2 - stepDistY;
        circle->velY *= -1;
    } else if (circle->posY - CIRCLE_SIZE / 2 < -stepDistY && circle->velY < 0.0f) {
        circle->posY = CIRCLE_SIZE / 2 - stepDistY;
        circle->velY *= -1;
    }

    circle->posX += circle->velX * dt;
    circle->posY += circle->velY * dt;
}

void checkPosition(struct Circle* circle) {
    if (circle->posX + CIRCLE_SIZE / 2 > SCREEN_WIDTH) {
        circle->posX = SCREEN_WIDTH - CIRCLE_SIZE / 2;
    } else if (circle->posX - CIRCLE_SIZE / 2 < 0.0f) {
        circle->posX = CIRCLE_SIZE / 2;
    }

    if (circle->posY + CIRCLE_SIZE / 2 > SCREEN_HEIGHT) {
        circle->posY = SCREEN_HEIGHT - CIRCLE_SIZE / 2;
    } else if (circle->posY - CIRCLE_SIZE / 2 < 0.0f) {
        circle->posY = CIRCLE_SIZE / 2;
    }
}

void checkCollisions(struct Cell* cell) {
    if (!cell->isLeaf) {
        for (int i = 0; i < 4; i++)
            checkCollisions(&cell->subcells[i]);
        return;
    }
    for (int i = 0; i < cell->numCirclesInCell - 1; i++) {
        int id_1 = cell->circle_ids[i];
        for (int j = i + 1; j < cell->numCirclesInCell; j++) {
            int id_2 = cell->circle_ids[j];
            if (fabsf(circles[id_1].posX - circles[id_2].posX) > CIRCLE_SIZE ||
                fabsf(circles[id_1].posY - circles[id_2].posY) > CIRCLE_SIZE)
                continue;
            if (circles[id_1].isUsed == 1 || circles[id_2].isUsed == 1)
                continue;
            circles[id_1].isUsed = 1;
            circles[id_2].isUsed = 1;
            float dist = (float) sqrt(pow(circles[id_2].posX - circles[id_1].posX, 2) +
                                      pow(circles[id_2].posY - circles[id_1].posY, 2));
            float sum_r = CIRCLE_SIZE;

            if (dist < sum_r) {
                float d = dist - sum_r;
                float dx = (circles[id_2].posX - circles[id_1].posX) / dist;
                float dy = (circles[id_2].posY - circles[id_1].posY) / dist;

                circles[id_1].posX += d * dx;
                circles[id_1].posY += d * dy;
                circles[id_2].posX -= d * dx;
                circles[id_2].posY -= d * dy;

                float v1x = circles[id_1].velX;
                float v1y = circles[id_1].velY;
                float v2x = circles[id_2].velX;
                float v2y = circles[id_2].velY;

                float v1x_new = v1x - 2 * CIRCLE_SIZE / sum_r * (v1x * dx + v1y * dy - v2x * dx - v2y * dy) * dx;
                float v1y_new = v1y - 2 * CIRCLE_SIZE / sum_r * (v1x * dx + v1y * dy - v2x * dx - v2y * dy) * dy;
                float v2x_new = v2x - 2 * CIRCLE_SIZE / sum_r * (v2x * dx + v2y * dy - v1x * dx - v1y * dy) * dx;
                float v2y_new = v2y - 2 * CIRCLE_SIZE / sum_r * (v2x * dx + v2y * dy - v1x * dx - v1y * dy) * dy;

                circles[id_1].velX = v1x_new;
                circles[id_1].velY = v1y_new;
                circles[id_2].velX = v2x_new;
                circles[id_2].velY = v2y_new;
                checkPosition(&circles[id_2]);
            }
            circles[id_2].isUsed = 0;
        }
        checkPosition(&circles[id_1]);
        circles[id_1].isUsed = 0;
    }
}

bool cellContainsCircle(struct Cell* cell, int circle_id) {
    for (int i = 0; i < cell->numCirclesInCell; i++)
        if (circle_id == cell->circle_ids[i])
            return true;
    return false;
}

bool isCircleOverCellArea(int circle_id, struct Cell* cell) {
    return circles[circle_id].posX + CIRCLE_SIZE / 2 >= cell->posX && circles[circle_id].posX - CIRCLE_SIZE / 2 <= cell->posX + cell->cellWidth && circles[circle_id].posY + CIRCLE_SIZE / 2 >= cell->posY && circles[circle_id].posY - CIRCLE_SIZE / 2 <= cell->posY + cell->cellHeight;
}

bool isFullCircleInsideCellArea(int circle_id, struct Cell* cell) {
    return circles[circle_id].posX - CIRCLE_SIZE / 2 >= cell->posX && circles[circle_id].posX + CIRCLE_SIZE / 2 <= cell->posX + cell->cellWidth && circles[circle_id].posY - CIRCLE_SIZE / 2 >= cell->posY && circles[circle_id].posY + CIRCLE_SIZE / 2 <= cell->posY + cell->cellHeight;
}

void collapse(struct Cell* cell) {
    if (cell->isLeaf)
        return;

    for (int i = 0; i < 4; i++)
        if (!cell->subcells[i].isLeaf)
            return;

    int numCirclesAdded = 0;
    int* addedCircles = (int*)malloc(MAX_CIRCLES_PER_CELL * sizeof(int));
    if (addedCircles == NULL) {
        printf("Memory error!");
        exit(1);
    }

    numCirclesAdded = 0;
    for (int i = 0; i < 4; i++) {
        struct Cell* subcell = &cell->subcells[i];
        for (int j = 0; j < subcell->numCirclesInCell; j++) {
            if (!isCircleOverCellArea(subcell->circle_ids[j], subcell))
                continue;
            if (!cellContainsCircle(subcell, subcell->circle_ids[j]))
                continue;
            if (numCirclesAdded >= MAX_CIRCLES_PER_CELL) {
                free(addedCircles);
                return;
            }
            printf("collapsed %d\n", subcell->circle_ids[j]);
            addedCircles[numCirclesAdded] = subcell->circle_ids[j];
            numCirclesAdded++;
        }
    }
    printf("\n\n");

    cell->circle_ids = addedCircles;
    cell->numCirclesInCell = numCirclesAdded;

    for (int i = 0; i < 4; i++) {
        struct Cell* subcell = &cell->subcells[i];
        free(subcell->circle_ids);
    }

    free(cell->subcells);
    cell->subcells = NULL;
    cell->isLeaf = true;
}

void split(struct Cell* cell) {
    if (!cell->isLeaf)
        return;

    cell->subcells = (struct Cell*)malloc(4 * sizeof(struct Cell));
    if (cell->subcells == NULL) {
        printf("Memory error!");
        exit(1);
    }

    cell->subcells[0].posX = cell->posX;
    cell->subcells[0].posY = cell->posY;
    cell->subcells[1].posX = cell->posX + cell->cellWidth / 2;
    cell->subcells[1].posY = cell->posY;
    cell->subcells[2].posX = cell->posX + cell->cellWidth / 2;
    cell->subcells[2].posY = cell->posY + cell->cellHeight / 2;
    cell->subcells[3].posX = cell->posX;
    cell->subcells[3].posY = cell->posY + cell->cellHeight / 2;

    for (int i = 0; i < 4; i++) {
        struct Cell *subcell = &cell->subcells[i];
        subcell->cellWidth = cell->cellWidth / 2;
        subcell->cellHeight = cell->cellHeight / 2;
        subcell->isLeaf = true;
        subcell->subcells = NULL;
        subcell->selected = false;
        subcell->parentCell = cell;
        subcell->numCirclesInCell = 0;
        subcell->circle_ids = (int *) malloc(MAX_CIRCLES_PER_CELL * sizeof(int));
        if (subcell->circle_ids == NULL) {
            printf("Memory error!");
            exit(1);
        }
        for (int j = 0; j < cell->numCirclesInCell; j++) {
            int circle_id = cell->circle_ids[j];
            if (!isCircleOverCellArea(circle_id, subcell))
                continue;
            subcell->circle_ids[subcell->numCirclesInCell] = circle_id;
            subcell->numCirclesInCell++;
        }
    }

    cell->isLeaf = false;
    free(cell->circle_ids);
    cell->circle_ids = NULL;
}

void addCircleToCell(int circle_id, struct Cell* cell) {
    if (!isCircleOverCellArea(circle_id, cell))
        return;

    if (cell->isLeaf) {
        if (cellContainsCircle(cell, circle_id))
            return;

        if (cell->numCirclesInCell >= MAX_CIRCLES_PER_CELL) {
            int s = cell->numCirclesInCell + 1;
            cell->circle_ids = (int *) realloc(cell->circle_ids, (s) * sizeof(int));
            if (cell->circle_ids == NULL) {
                printf("Memory error!");
                exit(1);
            }
        }

        cell->circle_ids[cell->numCirclesInCell] = circle_id;
        cell->numCirclesInCell++;
    } else {
        for (int i = 0; i < 4; i++)
            addCircleToCell(circle_id, &cell->subcells[i]);
    }
}

void addCircleToParentCell(int circle_id, struct Cell* cell) {
    struct Cell* parentCell = cell->parentCell;

    if (parentCell == NULL) {
        printf("OUTSIDE OF ROOTCELL!\n");
        addCircleToCell(circle_id, rootCell);
        return;
    }

    if (isCircleOverCellArea(circle_id, parentCell)) {
        for (int i = 0; i < 4; i++) {
            struct Cell* subcell = &parentCell->subcells[i];
            if (subcell == cell)
                continue;
            addCircleToCell(circle_id, parentCell);
        }
        if (isFullCircleInsideCellArea(circle_id, parentCell)) {
            printf("Assigned cell to circle number %d\n", circle_id);
            return;
        }
    }

    if (parentCell->parentCell != NULL)
        addCircleToParentCell(circle_id, parentCell);
}

void cleanup(struct Cell* cell) {
    bool selectedCircleInCell = false;
    for (int i = 0; i < cell->numCirclesInCell; i++) {
        int circle_id = cell->circle_ids[i];
        if (isFullCircleInsideCellArea(circle_id, cell)) {
            if (circle_id == selectedCircle)
                selectedCircleInCell= true;
            continue;
        }
        if (!isCircleOverCellArea(circle_id, cell)) {
            for (int j = i; j < cell->numCirclesInCell-1; j++) {
                cell->circle_ids[j] = cell->circle_ids[j + 1];
            }
            cell->numCirclesInCell--;
            i--;
        } else {
            if (circle_id == selectedCircle)
                selectedCircleInCell= true;
        }
        addCircleToParentCell(circle_id, cell);
    }
    cell->selected = selectedCircleInCell;
}

void updateCell(struct Cell* cell) {
    if (cell->isLeaf) {
        cleanup(cell);
        if (cell->numCirclesInCell > MAX_CIRCLES_PER_CELL && !(cell->cellWidth < 2 * (CIRCLE_SIZE) || cell->cellHeight < 2 * (CIRCLE_SIZE))) {
            split(cell);
        } else {
            checkCollisions(cell);
        }
    } else {
        //Um sicher zu gehen 3 einzelne for-loops, später reicht vlt einer
        for (int i = 0; i < 4; i++) {
            updateCell(&cell->subcells[i]);
        }

        for (int i = 0; i < 4; i++) {
            if (!cell->subcells[i].isLeaf)
                return;
            cleanup(&cell->subcells[i]);
        }

        collapse(cell);
    }
}

float random_float(float min, float max) {
    return (max - min) * ( (float)rand() / (float)RAND_MAX ) + min;
}

// Wenn Collapste Zellen neu gesplitted/circles hinzugefügt werden gibts probleme