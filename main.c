#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <GL/glut.h>
#include <GL/gl.h>

#define SCREEN_WIDTH 200.0f
#define SCREEN_HEIGHT 200.0f
#define numCircles 15
#define circleSize 10.0f
#define maxCirclesPerCell 3
#define maxSpawnSpeed 1.0f
#define maxSpeed 1.0f
#define gravity 0.1f

bool gravityState = true; //Mouseclick ins Fenster
bool drawCells = true; //Zeichnet tiefste Zellen des Baums
float dt = 0.1f;
int selectedCircle = 1;
int clearTimer = 100;

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
    struct Cell* subcells;
    struct Cell* parentCell;
    bool selected;
    int clearTimer;
};

struct Cell* rootCell;

void move(int circle_id);
bool addCircleToCell(int circle_id, struct Cell* cell, int depth);
bool isCircleOverlappingCellArea(int circle_id, struct Cell* cell);
void split(struct Cell* cell, int depth);
float random_float(float min, float max);
void checkCollisions(struct Cell* cell, int depth);
void updateCell(struct Cell* cell, int depth);
void printTree(struct Cell* cell, int depth);

void mouseClick(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        gravityState = !gravityState;
        printTree(rootCell, 0);
        /*printf("%d; %d\n", x, y);
        for (int i = 0; i < numCircles; i++) {
            if (x > (int)circles[i].posX - circleSize / 2 && x < (int)circles[i].posX + circleSize / 2 &&
                y > (int)circles[i].posY - circleSize / 2 && y < (int)circles[i].posY + circleSize / 2) {
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
        for (int i = 0; i < cell->numCirclesInCell; i++) {
            struct Circle circle = circles[cell->circle_ids[i]];
            glBegin(GL_LINES);
            glVertex2f(circle.posX, circle.posY);  // Top-left point
            glVertex2f(cell->posX + cell->cellWidth / 2, cell->posY + cell->cellHeight / 2);   // Top-right point
            glEnd();
        }
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
    printf("%d\n", cell->numCirclesInCell);

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

    for (int i = 0; i < numCircles; i++) {
        GLfloat centerX = circles[i].posX;
        GLfloat centerY = circles[i].posY;
        GLfloat radius = circleSize/2;
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
    for (int i = 0; i < numCircles; i++) {
        move(i);
    }
    updateCell(rootCell, 0);
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
    rootCell->clearTimer = clearTimer;
    rootCell->circle_ids = (int*)malloc(maxCirclesPerCell * sizeof(int));
    if (rootCell->circle_ids == NULL) {
        printf("Memory error!");
        exit(1);
    }
    rootCell->subcells = NULL;
    //rootCell->subcell1 = NULL;
    //rootCell->subcell2 = NULL;
    //rootCell->subcell3 = NULL;
    //rootCell->subcell4 = NULL;

    for (int i = 0; i < numCircles; i++) {
        circles[i].posX = random_float(circleSize/2, SCREEN_WIDTH-circleSize/2);
        circles[i].posY = random_float(circleSize/2, SCREEN_HEIGHT-circleSize/2);
        circles[i].velX = random_float(-maxSpawnSpeed, maxSpawnSpeed);
        circles[i].velY = random_float(-maxSpawnSpeed, maxSpawnSpeed);
        circles[i].isUsed = 0;
    }

    for (int i = 0; i < numCircles; i++) {
        addCircleToCell(i, rootCell, 0);
        updateCell(rootCell, 0);
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
        circle->velY -= gravity;

    if (circle->velX > maxSpeed)
        circle->velX = maxSpeed;
    if (circle->velY > maxSpeed)
        circle->velY = maxSpeed;
    if (circle->velX < -maxSpeed)
        circle->velX = -maxSpeed;
    if (circle->velY < -maxSpeed)
        circle->velY = -maxSpeed;

    float stepDistX = circle->velX * dt;
    float stepDistY = circle->velY * dt;

    if (circle->posX + circleSize/2 > SCREEN_WIDTH - stepDistX && circle->velX > 0.0f) {
        circle->posX = SCREEN_WIDTH - circleSize/2 - stepDistX;
        circle->velX *= -1;
    } else if (circle->posX - circleSize/2 < -stepDistX && circle->velX < 0.0f) {
        circle->posX = circleSize/2 - stepDistX;
        circle->velX *= -1;
    }

    if (circle->posY + circleSize/2 > SCREEN_HEIGHT - stepDistY && circle->velY > 0.0f) {
        circle->posY = SCREEN_HEIGHT - circleSize/2 - stepDistY;
        circle->velY *= -1;
    } else if (circle->posY - circleSize/2 < -stepDistY && circle->velY < 0.0f) {
        circle->posY = circleSize/2 - stepDistY;
        circle->velY *= -1;
    }

    circle->posX += circle->velX * dt;
    circle->posY += circle->velY * dt;
}

void checkPosition(struct Circle* circle) {
    if (circle->posX + circleSize/2 > SCREEN_WIDTH) {
        circle->posX = SCREEN_WIDTH - circleSize/2;
    } else if (circle->posX - circleSize/2 < 0.0f) {
        circle->posX = circleSize/2;
    }

    if (circle->posY + circleSize/2 > SCREEN_HEIGHT) {
        circle->posY = SCREEN_HEIGHT - circleSize/2;
    } else if (circle->posY - circleSize/2 < 0.0f) {
        circle->posY = circleSize/2;
    }
}

void checkCollisions(struct Cell* cell, int depth) {
    if (!cell->isLeaf) {
        for (int i = 0; i < 4; i++)
            checkCollisions(&cell->subcells[i], depth + 1);
        return;
    }
    for (int i = 0; i < cell->numCirclesInCell - 1; i++) {
        int id_1 = cell->circle_ids[i];
        for (int j = i + 1; j < cell->numCirclesInCell; j++) {
            int id_2 = cell->circle_ids[j];
            if (fabsf(circles[id_1].posX - circles[id_2].posX) > circleSize ||
                fabsf(circles[id_1].posY - circles[id_2].posY) > circleSize)
                continue;
            if (circles[id_1].isUsed == 1 || circles[id_2].isUsed == 1)
                continue;
            circles[id_1].isUsed = 1;
            circles[id_2].isUsed = 1;
            float dist = (float) sqrt(pow(circles[id_2].posX - circles[id_1].posX, 2) +
                                      pow(circles[id_2].posY - circles[id_1].posY, 2));
            float sum_r = circleSize;

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

                float v1x_new = v1x - 2 * circleSize / sum_r * (v1x * dx + v1y * dy - v2x * dx - v2y * dy) * dx;
                float v1y_new = v1y - 2 * circleSize / sum_r * (v1x * dx + v1y * dy - v2x * dx - v2y * dy) * dy;
                float v2x_new = v2x - 2 * circleSize / sum_r * (v2x * dx + v2y * dy - v1x * dx - v1y * dy) * dx;
                float v2y_new = v2y - 2 * circleSize / sum_r * (v2x * dx + v2y * dy - v1x * dx - v1y * dy) * dy;

                circles[id_1].velX = v1x_new;
                circles[id_1].velY = v1y_new;
                circles[id_2].velX = v2x_new;
                circles[id_2].velY = v2y_new;
            }
            checkPosition(&circles[id_2]);
            circles[id_2].isUsed = 0;
        }
        checkPosition(&circles[id_1]);
        circles[id_1].isUsed = 0;
    }
}

bool cellContainsCircle(struct Cell* cell, int circle_id) {
    if (cell->isLeaf) {
        for (int i = 0; i < cell->numCirclesInCell; i++)
            if (circle_id == cell->circle_ids[i])
                return true;
    } else {
        for (int i = 0; i < 4; i++)
            if (cellContainsCircle(&cell->subcells[i], circle_id))
                return true;
    }
    return false;
}

bool isCircleOverlappingCellArea(int circle_id, struct Cell* cell) {
    return circles[circle_id].posX + circleSize / 2 >= cell->posX && circles[circle_id].posX - circleSize / 2 <= cell->posX + cell->cellWidth && circles[circle_id].posY + circleSize / 2 >= cell->posY && circles[circle_id].posY - circleSize / 2 <= cell->posY + cell->cellHeight;
}

bool isFullCircleInsideCellArea(int circle_id, struct Cell* cell) {
    return circles[circle_id].posX - circleSize / 2 >= cell->posX && circles[circle_id].posX + circleSize / 2 <= cell->posX + cell->cellWidth && circles[circle_id].posY - circleSize / 2 >= cell->posY && circles[circle_id].posY + circleSize / 2 <= cell->posY + cell->cellHeight;
}

void sanityCheck(struct Cell* cell, struct Cell* exampleCell) {
    if (cell->isLeaf)
        for (int i = 0; i < cell->numCirclesInCell; i++) {
            int circle_id = cell->circle_ids[i];
            if (!cellContainsCircle(exampleCell, circle_id))
                exampleCell->circle_ids[exampleCell->numCirclesInCell++] = circle_id;
        }
    else
        for (int i = 0; i < 4; i++)
            sanityCheck(&cell->subcells[i], exampleCell);
}

void performSanityCheck(struct Cell* cell) {
    struct Cell *newCell = (struct Cell *) malloc(sizeof(struct Cell));
    newCell->numCirclesInCell = 0;
    newCell->circle_ids = (int *) malloc(numCircles * sizeof(int));
    newCell->posX = cell->posX;
    newCell->posY = cell->posY;
    newCell->cellWidth = cell->cellWidth;
    newCell->cellHeight = cell->cellHeight;
    newCell->isLeaf = true;
    newCell->parentCell = NULL;
    sanityCheck(cell, newCell);
    if (cell->numCirclesInCell != newCell->numCirclesInCell) {
        printf("NOT EQUAL!");
    }
    cell->numCirclesInCell = newCell->numCirclesInCell;
    free(newCell->circle_ids);
    free(newCell);
}

void collapse(struct Cell* cell, struct Cell* originCell, int depth) {
    for (int i = 0; i < depth; i++) {
        printf("- ");
    }
    printf("collapse\n");

    if (cell == NULL) {
        return;
    }

    performSanityCheck(cell);

    if (cell == originCell) {
        if (cell->isLeaf) {
            return;
        }
        if (cell->clearTimer > 0) {
            cell->clearTimer--;
            return;
        }
        cell->numCirclesInCell = 0;
        cell->circle_ids = (int *) malloc(maxCirclesPerCell * sizeof(int));
        if (cell->circle_ids == NULL) {
            printf("Error: Failed to allocate memory for circle ids in cell.\n");
            exit(1);
        }
    }

    if (cell->isLeaf) {
        for (int i = 0; i < cell->numCirclesInCell; i++) {
            int circle_id = cell->circle_ids[i];
            if (!isFullCircleInsideCellArea(circle_id, originCell))
                continue;
            if (cellContainsCircle(originCell, circle_id))
                continue;
            if (originCell->numCirclesInCell > maxCirclesPerCell) {
                printf("WTF!\n");
                for (int k = 0; k < depth; k++) {
                    printf("- ");
                }
                printf(">originCell has more than maxCirclesPerCell circles after collapse... ERROR\n");
                return;
                //selectedCircle = circle_id;
                //originCell->selected = true;
                //cell->selected = true;
            } else {
                originCell->circle_ids[originCell->numCirclesInCell++] = circle_id;
            }
        }
        free(cell->circle_ids);
        cell->circle_ids = NULL;
        cell->isLeaf = false;
    } else {
        for (int i = 0; i < 4; i++) {
            struct Cell *subcell = &cell->subcells[i];
            collapse(subcell, originCell, depth + 1);
        }
        free(cell->subcells);
        cell->subcells = NULL;
    }

    if (cell == originCell) {
        cell->isLeaf = true;
        performSanityCheck(cell);
    }
}

void split(struct Cell* cell, int depth) {
    for (int i = 0; i < depth; i++) {
        printf("- ");
    }
    printf("split\n");

    cell->subcells = (struct Cell*)malloc(4 * sizeof(struct Cell));
    if (cell->subcells == NULL) {
        printf("Memory error!");
        exit(1);
    }

    performSanityCheck(cell);

    cell->clearTimer = clearTimer;
    cell->subcells[0].posX = cell->posX;
    cell->subcells[0].posY = cell->posY;
    cell->subcells[1].posX = cell->posX + cell->cellWidth / 2;
    cell->subcells[1].posY = cell->posY;
    cell->subcells[2].posX = cell->posX + cell->cellWidth / 2;
    cell->subcells[2].posY = cell->posY + cell->cellHeight / 2;
    cell->subcells[3].posX = cell->posX;
    cell->subcells[3].posY = cell->posY + cell->cellHeight / 2;

    for (int i = 0; i < 4; i++) {
        cell->subcells[i].cellWidth = cell->cellWidth / 2;
        cell->subcells[i].cellHeight = cell->cellHeight / 2;
        cell->subcells[i].isLeaf = true;
        cell->subcells[i].numCirclesInCell = 0;
        cell->subcells[i].parentCell = cell;
        cell->subcells[i].subcells = NULL;
        cell->subcells[i].circle_ids = (int*)malloc(maxCirclesPerCell * sizeof(int));
        cell->subcells[i].selected = false;
        cell->subcells[i].clearTimer = clearTimer;
        if (cell->subcells[i].circle_ids == NULL) {
            printf("Memory error!");
            exit(1);
        }
    }

    int circlesAdded = 0;
    for (int i = 0; i < cell->numCirclesInCell; i++) {
        bool circleAdded = false;
        int circle_id = cell->circle_ids[i];
        for (int j = 0; j < 4; j++) {
            struct Cell *subcell = &cell->subcells[j];
            if (addCircleToCell(circle_id, subcell, depth+1))
                circleAdded = true;
        }
        if (circleAdded)
            circlesAdded++;
    }

    cell->isLeaf = false;
    free(cell->circle_ids);
    cell->circle_ids = NULL;
    //cell->numCirclesInCell = circlesAdded;

    performSanityCheck(cell);
}

bool addCircleToCell(int circle_id, struct Cell* cell, int depth) {
    for (int i = 0; i < depth; i++) {
        printf("- ");
    }
    printf("addCircleToCell\n");

    bool circleAdded = false;

    performSanityCheck(cell);

    if (!isCircleOverlappingCellArea(circle_id, cell)) {
        printf("NOT OVERLAPPING circle_id: %d\n", circle_id);
        circleAdded = false;
    } else {
        if (cell->isLeaf) {
            if (cellContainsCircle(cell, circle_id)) {
                printf("CELL CONTAINS circle_id: %d\n", circle_id);
                circleAdded = false;
            } else {
                if (cell->numCirclesInCell >= maxCirclesPerCell) {
                    cell->circle_ids = (int *) realloc(cell->circle_ids, ((cell->numCirclesInCell) + 1) * sizeof(int));
                    if (cell->circle_ids == NULL) {
                        printf("Memory error!");
                        exit(1);
                    }
                }
                cell->circle_ids[cell->numCirclesInCell] = circle_id;
                circleAdded = true;
                printf("added to this\n");
            }
            if (circleAdded)
                cell->numCirclesInCell++;
        } else {
            bool alreadyInsideCellArea = false;
            if (cellContainsCircle(cell, circle_id))
                alreadyInsideCellArea = true;
            for (int i = 0; i < 4; i++) {
                if (addCircleToCell(circle_id, &cell->subcells[i], depth + 1)) {
                    circleAdded = true;
                    printf("added to subcell %d : ", i);
                }
            }
            if (circleAdded && !alreadyInsideCellArea)
                cell->numCirclesInCell++;
            else
                printf("YO");
        }
    }

    performSanityCheck(cell);

    return circleAdded;
}

void addCircleToParentCell(int circle_id, struct Cell* cell, int depth) {
    for (int i = 0; i < depth; i++) {
        printf("- ");
    }
    printf("addCircleToParentCell\n");

    if (cell == rootCell)
        return;

    struct Cell* parentCell = cell->parentCell;

    if (isCircleOverlappingCellArea(circle_id, parentCell)) {
        for (int i = 0; i < 4; i++) {
            if (&parentCell->subcells[i] == cell) {
                continue;
            }
            struct Cell* neighbourCell = &parentCell->subcells[i];
            if (isCircleOverlappingCellArea(circle_id, neighbourCell))
                addCircleToCell(circle_id, neighbourCell, depth+1);
        }

        if (isFullCircleInsideCellArea(circle_id, parentCell)) {
            performSanityCheck(cell);
            performSanityCheck(parentCell);
            return;
        }
    } else {
        parentCell->numCirclesInCell--;
        performSanityCheck(cell);
        performSanityCheck(parentCell);
        //if (parentCell->numCirclesInCell <= maxCirclesPerCell)
            //collapse(parentCell, parentCell, depth);
        //if (parentCell->numCirclesInCell == 0)
            //printf("YO");
    }
    addCircleToParentCell(circle_id, parentCell, depth-1);
    performSanityCheck(cell);
    performSanityCheck(parentCell);
}

void updateCell(struct Cell* cell, int depth) {
    for (int i = 0; i < depth; i++) {
        printf("- ");
    }
    printf("updateCell\n");

    performSanityCheck(cell);

    if (cell->isLeaf) {
        bool circleInCell = false;
        for (int i = 0; i < cell->numCirclesInCell; i++) {
            int circle_id = cell->circle_ids[i];
            if (circle_id == selectedCircle)
                circleInCell = true;
            if (isFullCircleInsideCellArea(circle_id, cell)) {
                continue;
            }
            if (!isCircleOverlappingCellArea(circle_id, cell)) {
                for (int j = i; j < cell->numCirclesInCell-1; j++) {
                    cell->circle_ids[j] = cell->circle_ids[j + 1];
                }
                cell->numCirclesInCell--;
                i--;
            }
            addCircleToParentCell(circle_id, cell, depth);
        }
        cell->selected = circleInCell;

        performSanityCheck(cell);

        if (cell->numCirclesInCell > maxCirclesPerCell && !(cell->cellWidth < 2 * (circleSize + maxSpeed * dt) || cell->cellHeight < 4 * (circleSize + maxSpeed * dt))) {
            performSanityCheck(cell);
            split(cell, depth);
            //updateCell(cell, depth);
            performSanityCheck(cell);
            return;
        }

        checkCollisions(cell, depth);
        return;
    }

    if (cell->numCirclesInCell <= maxCirclesPerCell) {
        performSanityCheck(cell);
        collapse(cell, cell, depth);
        performSanityCheck(cell);
        //updateCell(cell, depth);
        return;
    }

    for (int i = 0; i < 4; i++) {
        performSanityCheck(cell);
        updateCell(&cell->subcells[i], depth+1);
        performSanityCheck(cell);
        if (cell->isLeaf)
            return;
    }
}

float random_float(float min, float max) {
    return (max - min) * ( (float)rand() / (float)RAND_MAX ) + min;
}

// Wenn Collapste Zellen neu gesplitted/circles hinzugefügt werden gibts probleme