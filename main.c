#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <omp.h>

#define SCREEN_WIDTH 1000.0f
#define SCREEN_HEIGHT 1000.0f
#define numCircles 1000
#define circleSize 5.0f
#define maxCirclesPerCell 5
#define maxSpawnSpeed 1.0f
#define maxSpeed 1.0f
#define gravity 0.1f

bool gravityState = true; //Mouseclick ins Fenster
bool drawCells = true; //Zeichnet tiefste Zellen des Baums
int updateTime = 30;
float dt = 1.0f;

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
    //struct Cell* subcell1;
    //struct Cell* subcell2;
    //struct Cell* subcell3;
    //struct Cell* subcell4;
};

struct Cell* rootCell;

FILE* file;

void move(int circle_id);
bool addCircleToCell(int circle_id, struct Cell* cell);
bool isCircleOverlappingCellArea(int circle_id, struct Cell* cell);
bool isFullCircleOutsideCellArea(int circle_id, struct Cell* cell);
void split(struct Cell* cell);
float random_float(float min, float max);
void checkCollisions(struct Cell* cell);
void updateCell(struct Cell* cell);
void printTree(struct Cell* cell, int depth);

void mouseClick(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        gravityState = !gravityState;
        printTree(rootCell, 0);
    } else if (button == 4) {
        dt += 0.1f;
        printf("%f\n", dt);
    } else if (button == 3) {
        dt -= 0.1f;
        if (dt < 0.1f)
            dt = 0.1f;
        printf("%f\n", dt);
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
    if (!drawCells)
        return;
    if (cell->isLeaf) {
        glBegin(GL_LINE_LOOP);
        glVertex2f(cell->posX, cell->posY); // bottom left corner
        glVertex2f(cell->posX, cell->posY + cell->cellHeight); // top left corner
        glVertex2f(cell->posX + cell->cellWidth, cell->posY + cell->cellHeight); // top right corner
        glVertex2f(cell->posX + cell->cellWidth, cell->posY); // bottom right corner
        glEnd();
    } else {
        for (int i = 0; i < 4; i++) {
            drawTree(&cell->subcells[i]);
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

void update(int counter) {
    while(true) {
        for (int i = 0; i < numCircles; i++) {
            move(i);
        }
        updateCell(rootCell);
    }
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
        addCircleToCell(i, rootCell);
        //updateCell(rootCell);
    }

    /*glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutInitWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
    glutCreateWindow("Bouncing Circles");
    glutDisplayFunc(display);
    glutTimerFunc(16, update, 0);
    glutMouseFunc(mouseClick);
    glutMainLoop();*/
    update(0);

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
    } else if (circle->posX - circleSize/2 < 0.0f - stepDistX && circle->velX < 0.0f) {
        circle->posX = circleSize/2 - stepDistX;
        circle->velX *= -1;
    }

    if (circle->posY + circleSize/2 > SCREEN_HEIGHT - stepDistY && circle->velY > 0.0f) {
        circle->posY = SCREEN_HEIGHT - circleSize/2 - stepDistY;
        circle->velY *= -1;
    } else if (circle->posY - circleSize/2 < 0.0f - stepDistY && circle->velY < 0.0f) {
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
    for (int i = 0; i < cell->numCirclesInCell; i++)
        if (circle_id == cell->circle_ids[i])
            return true;
    return false;
}

bool isCircleOverlappingCellArea(int circle_id, struct Cell* cell) {
    return !isFullCircleOutsideCellArea(circle_id, cell);
}

bool isFullCircleInsideCellArea(int circle_id, struct Cell* cell) {
    return circles[circle_id].posX - circleSize / 2 >= cell->posX && circles[circle_id].posX + circleSize / 2 <= cell->posX + cell->cellWidth && circles[circle_id].posY - circleSize / 2 >= cell->posY && circles[circle_id].posY + circleSize / 2 <= cell->posY + cell->cellHeight;
}

bool isFullCircleOutsideCellArea(int circle_id, struct Cell* cell) {
    return circles[circle_id].posX + circleSize / 2 < cell->posX || circles[circle_id].posX - circleSize / 2 > cell->posX + cell->cellWidth || circles[circle_id].posY + circleSize / 2 < cell->posY || circles[circle_id].posY - circleSize / 2 > cell->posY + cell->cellHeight;
}

void collapse(struct Cell* cell, struct Cell* originCell) {
    cell->isLeaf = true;
    cell->circle_ids = (int*)malloc(maxCirclesPerCell * sizeof(int));
    if (cell->circle_ids == NULL) {
        printf("Error: Failed to allocate memory for circle ids in cell.\n");
        return;
    }

    cell->numCirclesInCell = 0;
    for (int i = 0; i < 4; i++) {
        struct Cell* subcell = &cell->subcells[i];
        if (!subcell->isLeaf)
            collapse(subcell, originCell);
        for (int j = 0; j < subcell->numCirclesInCell; j++) {
            int circle_id = subcell->circle_ids[j];
            if (cellContainsCircle(originCell, circle_id))
                continue;
            originCell->circle_ids[originCell->numCirclesInCell] = circle_id;
            originCell->numCirclesInCell++;
        }
        if (subcell->circle_ids == NULL)
            continue;
        free(subcell->circle_ids);
        subcell->circle_ids = NULL;
    }

    free(cell->subcells);
    cell->subcells = NULL;
}

void split(struct Cell* cell) {
    cell->isLeaf = false;
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
        cell->subcells[i].cellWidth = cell->cellWidth / 2;
        cell->subcells[i].cellHeight = cell->cellHeight / 2;
        cell->subcells[i].isLeaf = true;
        cell->subcells[i].numCirclesInCell = 0;
        cell->subcells[i].parentCell = cell;
        cell->subcells[i].subcells = NULL;
    }

    for (int i = 0; i < 4; i++) {
        int numCirclesToAdd = 0;
        struct Cell* subcell = &cell->subcells[i];
        for (int j = 0; j < cell->numCirclesInCell; j++) {
            int circle_id = cell->circle_ids[j];
            if (isCircleOverlappingCellArea(circle_id, subcell))
                numCirclesToAdd++;
        }
        subcell->circle_ids = (int*)malloc(numCirclesToAdd * sizeof(int));
        if (subcell->circle_ids == NULL) {
            printf("Memory error!");
            exit(1);
        }
    }

    int addedCircles = 0;
    for (int i = 0; i < cell->numCirclesInCell; i++) {
        bool circleAdded = false;
        for (int j = 0; j < 4; j++) {
            if (addCircleToCell(cell->circle_ids[i], &cell->subcells[j]))
                circleAdded = true;
        }
        if (circleAdded)
            addedCircles++;
    }

    free(cell->circle_ids);
    cell->circle_ids = NULL;
    cell->numCirclesInCell = addedCircles;
}

bool addCircleToCell(int circle_id, struct Cell* cell) {
    if (isFullCircleOutsideCellArea(circle_id, cell))
        return false;

    bool circleAdded = false;

    if (cell->isLeaf) {
        if (cellContainsCircle(cell, circle_id))
            return false;
        if (cell->numCirclesInCell >= maxCirclesPerCell) {
            cell->circle_ids = (int *) realloc(cell->circle_ids, ((cell->numCirclesInCell) + 1) * sizeof(int));
            if (cell->circle_ids == NULL) {
                printf("Memory error!");
                exit(1);
            }
        }
        cell->circle_ids[cell->numCirclesInCell] = circle_id;
        circleAdded = true;
        cell->numCirclesInCell++;
    } else {
        for (int i = 0; i < 4; i++) {
            if (addCircleToCell(circle_id, &cell->subcells[i])) {
                circleAdded = true;
                cell->numCirclesInCell++;
            }
        }
    }

    return circleAdded;
}

void addCircleToParentCell(int circle_id, struct Cell* cell) {
    if (cell == rootCell)
        return;

    struct Cell* parentCell = cell->parentCell;
    if (isCircleOverlappingCellArea(circle_id, parentCell)) {
        for (int i = 0; i < 4; i++) {
            if (&parentCell->subcells[i] == cell)
                continue;
            struct Cell* neighbourCell = &parentCell->subcells[i];
            if (isCircleOverlappingCellArea(circle_id, neighbourCell))
                addCircleToCell(circle_id, neighbourCell);
        }
        if (isFullCircleInsideCellArea(circle_id, parentCell))
            return;
    } else {
        parentCell->numCirclesInCell--;
    }

    addCircleToParentCell(circle_id, parentCell);
}

int countCircles(struct Cell* cell) {
    if (cell->isLeaf)
        return cell->numCirclesInCell;

    int sum = 0;
    for (int i = 0; i < 4; i++) {
        sum += countCircles(&cell->subcells[i]);
    }
    return sum;
}

void updateCell(struct Cell* cell) {
    if (!cell->isLeaf) {
        if (cell->numCirclesInCell < maxCirclesPerCell) {
            collapse(cell, cell);
            //updateCell(cell);
        } else {
            for (int i = 0; i < 4; i++) {
                updateCell(&cell->subcells[i]);
                if (cell->isLeaf)
                    return;
            }
        }
        return;
    }

    if (cell->numCirclesInCell > maxCirclesPerCell && !(cell->cellWidth < 4 * circleSize || cell->cellHeight < 4 * circleSize)) {
        split(cell);
        return;
    }

    for (int i = 0; i < cell->numCirclesInCell; i++) {
        if (isFullCircleInsideCellArea(cell->circle_ids[i], cell))
            continue;
        int circle_id = cell->circle_ids[i];
        if (isFullCircleOutsideCellArea(circle_id, cell)) {
            cell->numCirclesInCell--;
            for (int j = i; j < cell->numCirclesInCell; j++) {
                cell->circle_ids[j] = cell->circle_ids[j + 1];
            }
        }
        addCircleToParentCell(circle_id, cell);
    }

    checkCollisions(cell);
}

float random_float(float min, float max) {
    return (max - min) * ( (float)rand() / (float)RAND_MAX ) + min;
}
