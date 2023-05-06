#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <omp.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 1000
#define numCircles 2000
#define circleSize 0.1f
#define maxCirclesPerCell 8
#define maxSpawnSpeed 1.0f
#define maxSpeed 1.0f
#define count 10000
#define saveIntervall 1
#define dt 10.0f
#define gravity 0.1f

int gravityState = 0; //Mouseclick ins Fenster
bool drawCells = true; //Zeichnet tiefste Zellen des Baums

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
    struct Cell* parentCell;
};

struct Cell* rootCell;

FILE* file;

// Define the number of sides of the polygon used to approximate the circle

void move(int circle_id);
int addCircleToCell(int circle_id, struct Cell* cell);
void deleteTree(struct Cell* cell);
bool isCircleOverlappingCellArea(int circle_id, struct Cell* cell);
void split(struct Cell* cell);
int random_int(int min, int max);
float random_float(float min, float max);
void save_Iteration(FILE* file);
void checkCollisions(struct Cell* cell);
void updateCell(struct Cell* cell);

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
            drawTree(&cell->subCells[i]);
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
        printTree(&cell->subCells[i], depth+1);
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
    //printTree(rootCell, 0);
    drawTree(rootCell);

    glutSwapBuffers();
}

void update(int counter) {
    updateCell(rootCell);
    for (int i = 0; i < numCircles; i++) {
        move(i);
    }
    glutPostRedisplay();
    glutTimerFunc(16, update, counter + 1);

    //if (counter % saveIntervall == 0)
        //save_Iteration(file);
}

int main(int argc, char** argv) {
    file = fopen("../data.txt","w");

    if(file == NULL) {
        printf("Error!");
        exit(1);
    }

    srand(90);

    rootCell = (struct Cell*)malloc(sizeof(struct Cell));
    rootCell->posX = 0.0f;
    rootCell->posY = 0.0f;
    rootCell->cellWidth = (float) SCREEN_WIDTH;
    rootCell->cellHeight = (float) SCREEN_HEIGHT;
    rootCell->isLeaf = true;
    rootCell->numCirclesInCell = 0;
    rootCell->parentCell = NULL;
    rootCell->circle_ids = (int*)malloc(maxCirclesPerCell * sizeof(int));

    printf("Flakesize: %f\nNum. Circles: %d\n", circleSize, numCircles);
    fprintf(file, "%d %d %d %d %d\n", SCREEN_WIDTH, SCREEN_HEIGHT, numCircles, (int)circleSize, count/saveIntervall);

    for (int i = 0; i < numCircles; i++) {
        circles[i].posX = random_float(circleSize/2, SCREEN_WIDTH-circleSize/2);
        circles[i].posY = random_float(circleSize/2, SCREEN_HEIGHT-circleSize/2);
        circles[i].velX = random_float(-maxSpawnSpeed, maxSpawnSpeed);
        circles[i].velY = random_float(-maxSpawnSpeed, maxSpawnSpeed);
        circles[i].isUsed = 0;
    }

    for (int i = 0; i < numCircles; i++) {
        addCircleToCell(i, rootCell);
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
    if (circle->velX > maxSpeed)
        circle->velX = maxSpeed;
    if (circle->velY > maxSpeed)
        circle->velY = maxSpeed;
    if (circle->velX < -maxSpeed)
        circle->velX = -maxSpeed;
    if (circle->velY < -maxSpeed)
        circle->velY = -maxSpeed;

    if (circle->posX + circleSize/2 > SCREEN_WIDTH) {
        circle->posX = SCREEN_WIDTH - circleSize/2 ;
        circle->velX *= -1;
    } else if (circle->posX - circleSize/2 < 0.0) {
        circle->posX = circleSize/2 ;
        circle->velX *= -1;
    }

    if (circle->posY +  circleSize/2  > SCREEN_HEIGHT) {
        circle->posY = SCREEN_HEIGHT - circleSize/2 ;
        circle->velY *= -1;
    } else if (circle->posY  - circleSize/2  < 0.0) {
        circle->posY = circleSize/2 ;
        circle->velY *= -1;
    }

    if (gravityState == 1)
        circle->velY -= gravity;

    circle->posX += circle->velX * dt;
    circle->posY += circle->velY * dt;
}

void checkCollisions(struct Cell* cell) {
    if (!cell->isLeaf) {
        for (int i = 0; i < 4; i++)
            checkCollisions(&cell->subCells[i]);
        return;
    }

    for (int i = 0; i < cell->numCirclesInCell; i++) {
        int id_1 = cell->circle_ids[i];
        for (int j = i + 1; j < cell->numCirclesInCell; j++) {
            int id_2 = cell->circle_ids[j];
            if (fabs(circles[id_1].posX - circles[id_2].posX) > circleSize ||
                fabs(circles[id_1].posY - circles[id_2].posY) > circleSize)
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
            circles[id_2].isUsed = 0;
        }
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
    return circles[circle_id].posX + circleSize / 2 >= cell->posX && circles[circle_id].posX - circleSize / 2 <= cell->posX + cell->cellWidth && circles[circle_id].posY + circleSize / 2 >= cell->posY && circles[circle_id].posY - circleSize / 2 <= cell->posY + cell->cellHeight;
}

bool isFullCircleInsideCellArea(int circle_id, struct Cell* cell) {
    return circles[circle_id].posX - circleSize / 2 > cell->posX && circles[circle_id].posX + circleSize / 2 < cell->posX + cell->cellWidth && circles[circle_id].posY - circleSize / 2 > cell->posY && circles[circle_id].posY + circleSize / 2 < cell->posY + cell->cellHeight;
}

bool isFullCircleOutsideCellArea(int circle_id, struct Cell* cell) {
    return circles[circle_id].posX + circleSize / 2 < cell->posX || circles[circle_id].posX - circleSize / 2 > cell->posX + cell->cellWidth || circles[circle_id].posY + circleSize / 2 < cell->posY || circles[circle_id].posY - circleSize / 2 > cell->posY + cell->cellHeight;
}

int* collapse(struct Cell* cell) {
    if (cell->isLeaf)
        return cell->circle_ids;

    cell->numCirclesInCell = 0;
    cell->circle_ids = (int*)malloc(maxCirclesPerCell * sizeof(int));

    for (int i = 0; i < 4; i++) {
        struct Cell* subCell = &cell->subCells[i];
        int* cur_subCell_circles = collapse(subCell);
        for (int j = 0; j < cell->subCells[i].numCirclesInCell; j++) {
            if (cellContainsCircle(cell, cur_subCell_circles[j]))
                continue;
            cell->circle_ids[cell->numCirclesInCell] = cur_subCell_circles[j];
            cell->numCirclesInCell++;
        }
        subCell->numCirclesInCell = 0;
        free(subCell->circle_ids);
    }

    cell->isLeaf = true;
    free(cell->subCells);
    cell->subCells = NULL;

    return cell->circle_ids;
}

void split(struct Cell* cell) {
    cell->subCells = (struct Cell*)malloc(4 * sizeof(struct Cell));
    if (cell->subCells == NULL) {
        printf("Memory error!");
        exit(1);
    }

    for (int i = 0; i < 4; i++) {
        cell->subCells[i].cellWidth = cell->cellWidth / 2;
        cell->subCells[i].cellHeight = cell->cellHeight / 2;
        cell->subCells[i].isLeaf = true;
        cell->subCells[i].numCirclesInCell = 0;
        cell->subCells[i].circle_ids = (int*)malloc(maxCirclesPerCell * sizeof(int));
        cell->subCells[i].parentCell = cell;

        for (int j = 0; j < cell->numCirclesInCell; j++) {
            if (isCircleOverlappingCellArea(cell->circle_ids[j], &cell->subCells[i])) {
                addCircleToCell(cell->circle_ids[j], &cell->subCells[i]);
            }
        }
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
    free(cell->circle_ids);
}

int addCircleToCell(int circle_id, struct Cell* cell) {
    int addedCircles = 0;

    if (!cell->isLeaf) {
        for (int i = 0; i < 4; i++)
            if (isCircleOverlappingCellArea(circle_id, &cell->subCells[i]))
                addedCircles += addCircleToCell(circle_id, &cell->subCells[i]);
    } else {
        if (cellContainsCircle(cell, circle_id))
            return 0;
        if (cell->numCirclesInCell <= maxCirclesPerCell || cell->cellWidth < 5 * circleSize ||
            cell->cellHeight < 5 * circleSize) {
            if (cell->numCirclesInCell >= maxCirclesPerCell)
                cell->circle_ids = (int*) realloc(cell->circle_ids, (cell->numCirclesInCell + 1) * sizeof(int));
            cell->circle_ids[cell->numCirclesInCell] = circle_id;
            addedCircles += 1;
        } else {
            split(cell);
            for (int i = 0; i < 4; i++) {
                if (isCircleOverlappingCellArea(circle_id, &cell->subCells[i]))
                    addedCircles += addCircleToCell(circle_id, &cell->subCells[i]);
            }
        }
    }
    if (addedCircles > 0)
        cell->numCirclesInCell += 1;
    return addedCircles;
}

void addCircleToParentCell(int circle_id, struct Cell* cell) {
    if (cell == NULL)
        return;

    if (isCircleOverlappingCellArea(circle_id, cell)) {
        addCircleToCell(circle_id, cell);
        return;
    }

    cell->numCirclesInCell--;
    addCircleToParentCell(circle_id, cell->parentCell);
}

void updateCell(struct Cell* cell) {
    if (!cell->isLeaf) {
        if (cell->numCirclesInCell <= maxCirclesPerCell) { // <=
            collapse(cell);
            checkCollisions(cell);
            return;
        }
        for (int i = 0; i < 4; i++)
            updateCell(&cell->subCells[i]);
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
        addCircleToParentCell(circle_id, cell->parentCell);
    }

    checkCollisions(cell);
}

int random_int(int min, int max) {
    return rand() % (max - min + 1) + min;
}

float random_float(float min, float max) {
    return (max - min) * ( (float)rand() / (float)RAND_MAX ) + min;
}

void save_Iteration(FILE* file) {
    for(int i=0; i<numCircles; i++) {
        fprintf(file,"%f %f ", circles[i].posX, circles[i].posY);
    }
    fprintf(file,"%s", " \n");
}
