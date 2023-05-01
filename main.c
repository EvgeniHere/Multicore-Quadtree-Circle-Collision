#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <GL/glut.h>
#include <GL/gl.h>


#define SCREEN_WIDTH 20
#define SCREEN_HEIGHT 20
#define numCircles ((SCREEN_HEIGHT - 2) / 2 * (SCREEN_WIDTH - 2) / 2)
#define circleSize 0.2f
#define maxCirclesPerCell 5
#define maxSpeed 1.0f
#define force 10.0
#define count 10
#define saveIntervall 100
#define dt 0.1f


struct Circle {
    double posX;
    double posY;
    double velX;
    double velY;
};

struct Circle circles[numCircles];

struct Cell* rootCell;

FILE* file;


struct Cell {
    double posX;
    double posY;
    double cellWidth;
    double cellHeight;
    int numCirclesInCell;
    bool isLeaf;
    int* circle_ids;
    struct Cell* subCells;
};

void move(int circle_id);
void addCircleToCell(int circle_id, struct Cell* cell, bool checkCollision);
void deleteTree(struct Cell* cell);
bool isCircleInCellArea(int circle_id, struct Cell cell);
void split(struct Cell* cell);
int random_int(int min, int max);
double random_double(double min, double max);
void save_Iteration(FILE* file);
void checkCollisions(int circle_id, struct Cell* cell);
void update(int counter);
void drawTree(struct Cell* cell);
void display();

int main(int argc, char** argv) {
    file = fopen("../data.txt","w");

    if(file == NULL) {
        printf("Error!");
        exit(1);
    }

    //circles =  (struct Circle*)malloc( numCircles * sizeof(struct Circle));

    srand(time(NULL));

    rootCell = (struct Cell*)malloc(sizeof(struct Cell));
    rootCell->cellWidth = (double) SCREEN_WIDTH;
    rootCell->cellHeight = (double) SCREEN_HEIGHT;
    rootCell->isLeaf = true;
    rootCell->numCirclesInCell = 0;
    rootCell->circle_ids = (int*)malloc(maxCirclesPerCell * sizeof(int));

    printf("Circle Size: %d\nNum. Circles: %d\n", (int)circleSize, numCircles);
    fprintf(file, "%d %d %d %d %d\n", SCREEN_WIDTH, SCREEN_HEIGHT, numCircles, (int)circleSize, count/saveIntervall);

    int i = 0;
    for (int x = 2; x <= SCREEN_WIDTH-2; x+=2) {
        for (int y = 2; y <= SCREEN_HEIGHT-2; y+=2) {
            circles[i].posX = x;
            circles[i].posY = y;
            circles[i].velX = random_double(-1.0, 1.0);
            circles[i].velY = random_double(-1.0, 1.0);
            addCircleToCell(i, rootCell, true);
            i++;
        }
    }
    save_Iteration(file);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutInitWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
    glutCreateWindow("Bouncing Circles");
    glutDisplayFunc(display);
    glutTimerFunc(16, update, 0);
    glutMainLoop();

    fclose(file);
    exit(0);
}

void drawCircle(GLdouble centerX, GLdouble centerY, GLdouble radius, int numSides) {
    GLdouble angleIncrement = 2.0 * M_PI / numSides;
    glBegin(GL_POLYGON);
    for (int i = 0; i < numSides; i++) {
        GLdouble angle = i * angleIncrement;
        GLdouble x = centerX + radius * cos(angle);
        GLdouble y = centerY + radius * sin(angle);
        glVertex2d(x, y);
    }
    glEnd();
}

void drawTree(struct Cell* cell) {
    if (cell->isLeaf) {
        glBegin(GL_LINE_LOOP);
        glVertex2d(cell->posX, cell->posY); // bottom left corner
        glVertex2d(cell->posX, cell->posY + cell->cellHeight); // top left corner
        glVertex2d(cell->posX + cell->cellWidth, cell->posY + cell->cellHeight); // top right corner
        glVertex2d(cell->posX + cell->cellWidth, cell->posY); // bottom right corner
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
        GLdouble centerX = circles[i].posX;
        GLdouble centerY = circles[i].posY;
        GLdouble radius = circleSize;
        int numSides = 32;
        drawCircle(centerX, centerY, radius, numSides);
    }

    //drawTree(rootCell);

    glutSwapBuffers();
}

void update(int counter) {
    deleteTree(rootCell);
    rootCell->circle_ids = (int*)malloc(maxCirclesPerCell * sizeof(int));

    for (int i = 0; i < numCircles; i++) {
        addCircleToCell(i, rootCell, true);
        move(i);
    }
    glutPostRedisplay();
    glutTimerFunc(16, update, counter + 1);

    if (counter % saveIntervall == 0)
        save_Iteration(file);
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

    circle->posX += circle->velX * dt;
    circle->posY += circle->velY * dt;
}

void checkCollisions(int circle_id, struct Cell* cell) {
    for (int i = 0; i < cell->numCirclesInCell; i++) {
        if (cell->circle_ids[i] == circle_id)
            continue;
        int j = cell->circle_ids[i];
        float dist = (float) sqrt(
                pow(circles[j].posX - circles[circle_id].posX, 2) + pow(circles[j].posY - circles[circle_id].posY, 2));
        float sum_r = circleSize * 2;

        if (dist < sum_r) {
            float d = dist - sum_r;
            float dx = (circles[j].posX - circles[circle_id].posX) / dist;
            float dy = (circles[j].posY - circles[circle_id].posY) / dist;

            circles[circle_id].posX += d * dx;
            circles[circle_id].posY += d * dy;
            circles[j].posX -= d * dx;
            circles[j].posY -= d * dy;

            float v1x = circles[circle_id].velX;
            float v1y = circles[circle_id].velY;
            float v2x = circles[j].velX;
            float v2y = circles[j].velY;

            float v1x_new = v1x - 2 * circleSize / sum_r * (v1x * dx + v1y * dy - v2x * dx - v2y * dy) * dx;
            float v1y_new = v1y - 2 * circleSize / sum_r * (v1x * dx + v1y * dy - v2x * dx - v2y * dy) * dy;
            float v2x_new = v2x - 2 * circleSize / sum_r * (v2x * dx + v2y * dy - v1x * dx - v1y * dy) * dx;
            float v2y_new = v2y - 2 * circleSize / sum_r * (v2x * dx + v2y * dy - v1x * dx - v1y * dy) * dy;

            circles[circle_id].velX = v1x_new;
            circles[circle_id].velY = v1y_new;
            circles[j].velX = v2x_new;
            circles[j].velY = v2y_new;

        }
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
    return circles[circle_id].posX + circleSize / 2 > cell.cellWidth && circles[circle_id].posX - circleSize / 2 < cell.posX + cell.cellWidth && circles[circle_id].posY + circleSize / 2 > cell.posY && circles[circle_id].posY - circleSize / 2 < cell.posY + cell.cellHeight;
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
                addCircleToCell(circle_id, &cell->subCells[i], false);
        return;
    }

    if (cell->numCirclesInCell < maxCirclesPerCell  || cell->cellWidth < 4*circleSize || cell->cellHeight < 4*circleSize) {
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

int random_int(int min, int max) {
    return rand() % (max - min + 1) + min;
}

double random_double(double min, double max) {
    return (max - min) * ( (double)rand() / (double)RAND_MAX ) + min;
}

void save_Iteration(FILE* file) {
    for(int i=0; i<numCircles; i++) {
        fprintf(file,"%f %f ", circles[i].posX, circles[i].posY);
    }
    fprintf(file,"%s", " \n");
}
