#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
//#include <GL/glut.h>
//#include <GL/gl.h>
#include <mpi.h>
#include <time.h>

#define SCREEN_WIDTH 1000.0
#define SCREEN_HEIGHT 1000.0
#define numCircles 6000
#define circleSize 5
#define maxCirclesPerCell 3
#define maxSpawnSpeed (circleSize / 4.0)
#define maxSpeed (circleSize / 4.0)
#define gravity 0.00

bool gravityState = false; //Mouseclick ins Fenster
bool drawCells = true; //Zeichnet tiefste Zellen des Baums
double dt = 1.0;
int selectedCircle = 5742;
double friction = 0.9;
double minCellSize = 2 * circleSize + 4 * maxSpeed;
clock_t begin;
int frames = 0;
int rank;
int world_size;

struct Circle {
    double posX;
    double posY;
    double velX;
    double velY;
};

struct Circle circles[numCircles];

struct Cell {
    double posX;
    double posY;
    double cellWidth;
    double cellHeight;
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
bool isCircleOverlappingCellArea(int circle_id, struct Cell* cell);
void split(struct Cell* cell);
double random_double(double min, double max);
void checkCollisions(int  circle_id, struct Cell* cell);
void updateCell(struct Cell* cell);
void printTree(struct Cell* cell, int depth);
bool deleteCircle(struct Cell* cell, int circle_id);
bool cellContainsCircle(struct Cell* cell, int circle_id);
void collapseAllCollapsableCells(struct Cell* cell);
/*
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
/*
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
        glVertex2i(centerX, centerY); //Set pixel coordinates
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
        /*for (int i = 0; i < cell->numCirclesInCell; i++) {
            struct Circle circle = circles[cell->circle_ids[i]];
            glBegin(GL_LINES);
            glVertex2f(circle.posX, circle.posY);  // Top-left point
            glVertex2f(cell->posX + cell->cellWidth / 2, cell->posY + cell->cellHeight / 2);   // Top-right point
            glEnd();
        }*/
/*
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
    printf("%d", cell->numCirclesInCell);

    if (cell->isLeaf) {
        printf(": ");
        for (int i = 0; i < cell->numCirclesInCell; i++) {
            printf("%d ", cell->circle_ids[i]);
        }
        printf("\n");
        return;
    }
    printf("\n");

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
        GLfloat radius = circleSize/2.0;
        if (radius < 1.0f)
            radius = 1.0f;
        int numSides = 32;
        //glColor3ub(circles[i].red, circles[i].green, circles[i].blue);


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
*/
void update() {
    int h = numCircles / world_size;
    for (int i = rank * h; i < (rank + 1) * h; i++) {
        checkCollisions(i, rootCell);
    }
    MPI_Allgather(&circles[(int)(rank * h)], (int)h * 4, MPI_DOUBLE, circles, (int)h * 4, MPI_DOUBLE, MPI_COMM_WORLD);
    for (int i = 0; i < numCircles; i++) {
        move(i);
        deleteCircle(rootCell, i);
    }
    updateCell(rootCell);
    collapseAllCollapsableCells(rootCell);
    //splitAllSplittableCells(rootCell);
    frames++;
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    if (time_spent >= 10) {
        printf("%f fps\n", frames/time_spent);
        frames = 0;
        begin = end;
        MPI_Finalize();
        exit(0);
    }

}

int main(int argc, char** argv) {
    srand(90);

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    double intpart;
    if(rank == 0 && modf(numCircles/(double)world_size, &intpart) != 0) {
        printf("%d %d Number of circles must be dividable by num of processes!\n", world_size, numCircles);
        MPI_Abort(MPI_COMM_WORLD, 3);
        exit(1);
    }
    rootCell = (struct Cell*)malloc(sizeof(struct Cell));
    if (rootCell == NULL) {
        printf("Memory error!");
        exit(1);
    }
    rootCell->posX = 0.0f;
    rootCell->posY = 0.0f;
    rootCell->cellWidth = SCREEN_WIDTH;
    rootCell->cellHeight = SCREEN_HEIGHT;
    rootCell->isLeaf = true;
    rootCell->numCirclesInCell = 0;
    rootCell->parentCell = NULL;
    rootCell->circle_ids = (int*)malloc(maxCirclesPerCell * sizeof(int));
    if (rootCell->circle_ids == NULL) {
        printf("Memory error!");
        exit(1);
    }
    rootCell->subcells = NULL;
    if(rank==0) {
        for (int i = 0; i < numCircles; i++) {
            circles[i].posX = random_double(circleSize / 2.0, SCREEN_WIDTH - circleSize / 2.0);
            circles[i].posY = random_double(circleSize / 2.0, SCREEN_HEIGHT - circleSize / 2.0);
            circles[i].velX = random_double(-maxSpawnSpeed, maxSpawnSpeed);
            circles[i].velY = random_double(-maxSpawnSpeed, maxSpawnSpeed);
        }
    }
    MPI_Bcast(circles, numCircles*4, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    for (int i = 0; i < numCircles; i++) {
        addCircleToCell(i, rootCell);
    }
    updateCell(rootCell);
    /*if(rank == 0) {
        glutInit(&argc, argv);
        glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
        glutInitWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
        glutCreateWindow("Bouncing Circles");
        glutDisplayFunc(display);
        begin = clock();
        glutTimerFunc(0, update, 0);
        glutMouseFunc(mouseClick);
        glutMainLoop();
    }
    else {
     */
    begin = clock();
        while(true) {
            update();
        }
    //}
    MPI_Finalize();
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

void checkCollisions(int  circle_id, struct Cell* cell) {
    if (!cell->isLeaf) {
        for (int i = 0; i < 4; i++) {
            if (isCircleOverlappingCellArea(circle_id, &cell->subcells[i])) {
                checkCollisions(circle_id, &cell->subcells[i]);
            }
        }
        return;
    }
    int id_1 = circle_id;

    for (int j = 0; j < cell->numCirclesInCell; j++) {
        int id_2 = cell->circle_ids[j];

        if(id_1 == id_2)
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
                //circles[id_2].velX -= dot * dx;
                //circles[id_2].velY -= dot * dy;

                circles[id_1].velX *= friction;
                circles[id_1].velY *= friction;
                //circles[id_2].velX *= friction;
                //circles[id_2].velY *= friction;
            }
        }
    }
    checkPosition(&circles[circle_id]);
}

bool isCircleCloseToCellArea(int circle_id, struct Cell* cell) {
    return circles[circle_id].posX + circleSize + maxSpeed >= cell->posX && circles[circle_id].posX - circleSize - maxSpeed <= cell->posX + cell->cellWidth && circles[circle_id].posY + circleSize + maxSpeed >= cell->posY && circles[circle_id].posY - circleSize - maxSpeed <= cell->posY + cell->cellHeight;
}

bool cellContainsCircle(struct Cell* cell, int circle_id) {
    if (cell->isLeaf) {
        for (int i = 0; i < cell->numCirclesInCell; i++)
            if (circle_id == cell->circle_ids[i])
                return true;
    } else {
        for (int i = 0; i < 4; i++) {
            if (isCircleCloseToCellArea(circle_id, &cell->subcells[i]))
                if (cellContainsCircle(&cell->subcells[i], circle_id))
                    return true;
        }
    }
    return false;
}

bool isCircleOverlappingCellArea(int circle_id, struct Cell* cell) {
    return circles[circle_id].posX + circleSize / 2.0 >= cell->posX && circles[circle_id].posX - circleSize / 2.0 <= cell->posX + cell->cellWidth && circles[circle_id].posY + circleSize / 2.0 >= cell->posY && circles[circle_id].posY - circleSize / 2.0 <= cell->posY + cell->cellHeight;
}

bool isFullCircleInsideCellArea(int circle_id, struct Cell* cell) {
    return circles[circle_id].posX - circleSize / 2.0 >= cell->posX && circles[circle_id].posX + circleSize / 2.0 <= cell->posX + cell->cellWidth && circles[circle_id].posY - circleSize / 2.0 >= cell->posY && circles[circle_id].posY + circleSize / 2.0 <= cell->posY + cell->cellHeight;
}

void collapse(struct Cell* cell, struct Cell* originCell) {
    if (cell == NULL) {
        return;
    }

    if (cell == originCell) {
        if (cell->isLeaf) {
            return;
        }
        cell->isLeaf = true;
        cell->numCirclesInCell = 0;
        cell->circle_ids = (int *) malloc(maxCirclesPerCell * sizeof(int));
        if (cell->circle_ids == NULL) {
            printf("Error: Failed to allocate memory for circle ids in cell.\n");
            exit(1);
        }
    } else if (cell->isLeaf) {
        for (int i = 0; i < cell->numCirclesInCell; i++) {
            int circle_id = cell->circle_ids[i];
            if (!isCircleOverlappingCellArea(circle_id, originCell) || cellContainsCircle(originCell, circle_id))
                continue;
            if (originCell->numCirclesInCell > maxCirclesPerCell) {
                //printTree(originCell, 0);
                /*for(int j = 0; j < 4; j++) {
                    printTree(&originCell->subcells[j], 0);
                }*/
                printf("WTF!\n");
                return;
            } else {
                originCell->circle_ids[originCell->numCirclesInCell++] = circle_id;
            }
        }
        free(cell->circle_ids);
        cell->circle_ids = NULL;
        return;
    }
    for (int i = 0; i < 4; i++) {
        struct Cell *subcell = &cell->subcells[i];
        collapse(subcell, originCell);
    }
    free(cell->subcells);
    cell->subcells = NULL;

    if (cell == originCell) {
        cell->isLeaf = true;
    }
}

void collapseAllCollapsableCells(struct Cell* cell) {
    if (cell->isLeaf)
        return;

    for (int i = 0; i < 4; i++)
        collapseAllCollapsableCells(&cell->subcells[i]);

    if (cell->numCirclesInCell <= maxCirclesPerCell)
        collapse(cell, cell);
}

void split(struct Cell* cell) {
    cell->subcells = (struct Cell*)malloc(4 * sizeof(struct Cell));
    if (cell->subcells == NULL) {
        printf("Memory error!");
        exit(1);
    }

    cell->subcells[0].posX = cell->posX;
    cell->subcells[0].posY = cell->posY;
    cell->subcells[1].posX = cell->posX + cell->cellWidth / 2.0;
    cell->subcells[1].posY = cell->posY;
    cell->subcells[2].posX = cell->posX + cell->cellWidth / 2.0;
    cell->subcells[2].posY = cell->posY + cell->cellHeight / 2.0;
    cell->subcells[3].posX = cell->posX;
    cell->subcells[3].posY = cell->posY + cell->cellHeight / 2.0;

    for (int i = 0; i < 4; i++) {
        cell->subcells[i].cellWidth = cell->cellWidth / 2.0;
        cell->subcells[i].cellHeight = cell->cellHeight / 2.0;
        cell->subcells[i].isLeaf = true;
        cell->subcells[i].numCirclesInCell = 0;
        cell->subcells[i].parentCell = cell;
        cell->subcells[i].subcells = NULL;
        cell->subcells[i].circle_ids = (int*)malloc(maxCirclesPerCell * sizeof(int));
        cell->subcells[i].selected = false;
        if (cell->subcells[i].circle_ids == NULL) {
            printf("Memory error!");
            exit(1);
        }
    }

    for (int i = 0; i < cell->numCirclesInCell; i++) {
        int circle_id = cell->circle_ids[i];
        for (int j = 0; j < 4; j++) {
            struct Cell* subcell = &cell->subcells[j];
            if (!isCircleOverlappingCellArea(circle_id, subcell))
                continue;
            addCircleToCell(circle_id, subcell);
        }
    }

    cell->isLeaf = false;
    free(cell->circle_ids);
    cell->circle_ids = NULL;
}

void addCircleToCell(int circle_id, struct Cell* cell) {
    if (cell->isLeaf) {
        if (cellContainsCircle(cell, circle_id))
            return;

        if (cell->numCirclesInCell >= maxCirclesPerCell) {
            if (cell->cellWidth > minCellSize && cell->cellHeight > minCellSize) {
                split(cell);
                for (int i = 0; i < 4; i++) {
                    struct Cell* subcell = &cell->subcells[i];
                    if (!isCircleOverlappingCellArea(circle_id, subcell))
                        continue;
                    addCircleToCell(circle_id, subcell);
                }
                cell->numCirclesInCell++;
                return;
            } else {
                cell->circle_ids = (int *) realloc(cell->circle_ids, ((cell->numCirclesInCell) + 1) * sizeof(int));
                if (cell->circle_ids == NULL) {
                    printf("Memory error!");
                    exit(1);
                }
            }
        }
        cell->circle_ids[cell->numCirclesInCell++] = circle_id;
        return;
    }

    bool alreadyInsideCell = cellContainsCircle(cell, circle_id);
    for (int i = 0; i < 4; i++) {
        struct Cell* subcell = &cell->subcells[i];
        if (!isCircleOverlappingCellArea(circle_id, subcell))
            continue;
        addCircleToCell(circle_id, subcell);
    }

    if (!alreadyInsideCell)
        cell->numCirclesInCell++;
}

void addCircleToParentCell(int circle_id, struct Cell* cell) {
    if (cell == rootCell)
        return;

    struct Cell* parentCell = cell->parentCell;

    if (isCircleOverlappingCellArea(circle_id, parentCell)) {
        for (int i = 0; i < 4; i++) {
            struct Cell* neighbourCell = &parentCell->subcells[i];
            if (neighbourCell == cell || !isCircleOverlappingCellArea(circle_id, neighbourCell))
                continue;
            addCircleToCell(circle_id, neighbourCell);
        }
        if (isFullCircleInsideCellArea(circle_id, parentCell))
            return;
    }

    addCircleToParentCell(circle_id, parentCell);
}

void updateCell(struct Cell* cell) {
    if (cell->isLeaf) {
        bool circleInCell = false;
        for (int i = 0; i < cell->numCirclesInCell; i++) {
            int circle_id = cell->circle_ids[i];
            if (circle_id == selectedCircle)
                circleInCell = true;
            if (isFullCircleInsideCellArea(circle_id, cell)) {
                continue;
            }
            /*if (!isCircleOverlappingCellArea(circle_id, cell)) {
                deleteCircle(rootCell, circle_id);
                i--;
            }*/
            addCircleToParentCell(circle_id, cell);
        }
        cell->selected = circleInCell;
        return;
    }

    for (int i = 0; i < 4; i++) {
        updateCell(&cell->subcells[i]);
    }
    //if (cell->numCirclesInCell <= maxCirclesPerCell)
        //collapse(cell, cell);
}

bool deleteCircle(struct Cell* cell, int circle_id) {
    if (cell->isLeaf) {
        if (!isCircleOverlappingCellArea(circle_id, cell)) {
            for (int i = 0; i < cell->numCirclesInCell; i++) {
                if (cell->circle_ids[i] != circle_id)
                    continue;
                for (int j = i; j < cell->numCirclesInCell - 1; j++) {
                    cell->circle_ids[j] = cell->circle_ids[j + 1];
                }
                cell->numCirclesInCell--;
                return true;
            }
        }
    } else {
        if (isCircleCloseToCellArea(circle_id, cell)) {
            bool deleted = false;
            for (int i = 0; i < 4; i++) {
                if (deleteCircle(&cell->subcells[i], circle_id))
                    deleted = true;
            }
            if (deleted && !isCircleOverlappingCellArea(circle_id, cell)) {
                cell->numCirclesInCell--;
                return true;
            }
        }
    }
    return false;
}

double random_double(double min, double max) {
    double range = max - min;
    double scaled = (double)rand() / RAND_MAX;  // random value between 0 and 1
    return min + (scaled * range);
}
