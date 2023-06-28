#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "circle.c"

struct Cell {
    double posX;
    double posY;
    double cellWidth;
    double cellHeight;
    int numCirclesInCell;
    bool isLeaf;
    struct Circle* circles;
    struct Cell* subcells;
    struct Cell* parentCell;
};

int maxCirclesPerCell;
double minCellSize;

struct Cell* rootCell;

int numCircles;
struct Circle* circles;

struct Process {
    int posX;
    int posY;
    int width;
    int height;
};

struct Process* processes;

int numProcesses;

void updateCell(struct Cell* cell);
void addCircleToCell(struct Circle* circle, struct Cell* cell);
void addCircleToParentCell(struct Circle* circle, struct Cell* cell);
void split(struct Cell* cell);
void collapse(struct Cell* cell, struct Cell* originCell);
bool deleteCircle(struct Cell* cell, struct Circle* circle);
void checkCollisions(struct Cell* cell);
bool cellContainsCircle(struct Cell* cell, struct Circle* circle);
bool isCircleFullInsideCellArea(struct Circle* circle, struct Cell* cell);
bool isCircleOverlappingCellArea(struct Circle* circle, struct Cell* cell);
bool isCircleCloseToCellArea(struct Circle* circle, struct Cell* cell);
bool isCircleOverlappingArea(struct Circle* circle, double cellPosX, double cellPosY, double cellWidth, double cellHeight);
void printTree(struct Cell* cell, int depth);
double random_double(double min, double max);

void setupQuadtree(double rootCellX, double rootCellY, double rootCellWidth, double rootCellHeight) {
    rootCell = (struct Cell*)malloc(sizeof(struct Cell));
    if (rootCell == NULL) {
        printf("Memory error!");
        exit(1);
    }
    rootCell->posX = rootCellX;
    rootCell->posY = rootCellY;
    rootCell->cellWidth = rootCellWidth;
    rootCell->cellHeight = rootCellHeight;
    rootCell->isLeaf = true;
    rootCell->numCirclesInCell = 0;
    rootCell->parentCell = NULL;
    rootCell->circles = (struct Circle*)malloc(maxCirclesPerCell * sizeof(struct Circle));
    if (rootCell->circles == NULL) {
        printf("Memory error!");
        exit(1);
    }
    rootCell->subcells = NULL;

    for (int i = 0; i < numCircles; i++) {
        addCircleToCell(&circles[i], rootCell);
    }
    updateCell(rootCell);
}

void updateCell(struct Cell* cell) {
    if (cell->isLeaf) {
        for (int i = 0; i < cell->numCirclesInCell; i++) {
            struct Circle* circle = &cell->circles[i];
            move(circle);
            if (isCircleFullInsideCellArea(circle, cell))
                continue;
            /*if (!isCircleOverlappingCellArea(circle_id, cell)) {
                deleteCircle(rootCell, circle_id);
                i--;
            }*/
            addCircleToParentCell(circle, cell);
        }
        return;
    }

    for (int i = 0; i < 4; i++) {
        updateCell(&cell->subcells[i]);
    }
    if (cell->numCirclesInCell <= maxCirclesPerCell)
        collapse(cell, cell);
}

void addCircleToCell(struct Circle* circle, struct Cell* cell) {
    if (!isCircleOverlappingCellArea(circle, cell))
        return;

    if (cell->isLeaf) {
        if (cellContainsCircle(cell, circle))
            return;

        if (cell->numCirclesInCell >= maxCirclesPerCell) {
            if (cell->cellWidth > minCellSize && cell->cellHeight > minCellSize) {
                split(cell);
                for (int i = 0; i < 4; i++) {
                    struct Cell* subcell = &cell->subcells[i];
                    if (!isCircleOverlappingCellArea(circle, subcell))
                        continue;
                    addCircleToCell(circle, subcell);
                }
                cell->numCirclesInCell++;
                return;
            } else {
                cell->circles = (struct Circle *) realloc(cell->circles, ((cell->numCirclesInCell) + 1) * sizeof(struct Circle));
                if (cell->circles == NULL) {
                    printf("Memory error!");
                    exit(1);
                }
            }
        }
        cell->circles[cell->numCirclesInCell++] = *circle;
        return;
    }

    bool alreadyInsideCell = cellContainsCircle(cell, circle);
    for (int i = 0; i < 4; i++) {
        struct Cell* subcell = &cell->subcells[i];
        if (!isCircleOverlappingCellArea(circle, subcell))
            continue;
        addCircleToCell(circle, subcell);
    }

    if (!alreadyInsideCell)
        cell->numCirclesInCell++;
}

void addCircleToParentCell(struct Circle* circle, struct Cell* cell) {
    if (cell == rootCell) {

        return;
    }

    struct Cell* parentCell = cell->parentCell;

    if (isCircleOverlappingCellArea(circle, parentCell)) {
        for (int i = 0; i < 4; i++) {
            struct Cell* neighbourCell = &parentCell->subcells[i];
            if (neighbourCell == cell || !isCircleOverlappingCellArea(circle, neighbourCell))
                continue;
            addCircleToCell(circle, neighbourCell);
        }
        if (isCircleFullInsideCellArea(circle, parentCell))
            return;
    }

    addCircleToParentCell(circle, parentCell);
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
        cell->subcells[i].circles = (struct Circle*)malloc(maxCirclesPerCell * sizeof(struct Circle));
        if (cell->subcells[i].circles == NULL) {
            printf("Memory error!");
            exit(1);
        }
    }

    for (int i = 0; i < cell->numCirclesInCell; i++) {
        struct Circle* circle = &cell->circles[i];
        for (int j = 0; j < 4; j++) {
            struct Cell* subcell = &cell->subcells[j];
            if (!isCircleOverlappingCellArea(circle, subcell))
                continue;
            addCircleToCell(circle, subcell);
        }
    }

    cell->isLeaf = false;
    free(cell->circles);
    cell->circles = NULL;
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
        cell->circles = (int *) malloc(maxCirclesPerCell * sizeof(struct Circle));
        if (cell->circles == NULL) {
            printf("Error: Failed to allocate memory for circle ids in cell.\n");
            exit(1);
        }
    } else if (cell->isLeaf) {
        for (int i = 0; i < cell->numCirclesInCell; i++) {
            struct Circle* circle = &cell->circles[i];
            if (!isCircleOverlappingCellArea(circle, originCell) || cellContainsCircle(originCell, circle))
                continue;
            if (originCell->numCirclesInCell > maxCirclesPerCell) {
                printTree(originCell, 0);
                for(int j = 0; j < 4; j++) {
                    printTree(&originCell->subcells[j], 0);
                }
                printf("WTF!\n");
                return;
            } else {
                originCell->circles[originCell->numCirclesInCell++] = *circle;
            }
        }
        free(cell->circles);
        cell->circles = NULL;
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

bool deleteCircle(struct Cell* cell, struct Circle* circle) {
    if (cell->isLeaf) {
        if (!isCircleOverlappingCellArea(circle, cell)) {
            for (int i = 0; i < cell->numCirclesInCell; i++) {
                if (&cell->circles[i] != circle)
                    continue;
                for (int j = i; j < cell->numCirclesInCell - 1; j++) {
                    cell->circles[j] = cell->circles[j + 1];
                }
                cell->numCirclesInCell--;
                return true;
            }
        }
    } else {
        if (isCircleCloseToCellArea(circle, cell)) {
            bool deleted = false;
            for (int i = 0; i < 4; i++) {
                if (deleteCircle(&cell->subcells[i], circle))
                    deleted = true;
            }
            if (deleted && !isCircleOverlappingCellArea(circle, cell)) {
                cell->numCirclesInCell--;
                return true;
            }
        }
    }
    return false;
}

void checkPosition(struct Circle* circle) {
    if (circle->posX + circleSize/2.0 > circle_max_X) {
        circle->posX = circle_max_X - circleSize/2.0;
    } else if (circle->posX - circleSize/2.0 < 0.0) {
        circle->posX = circleSize/2.0;
    }

    if (circle->posY + circleSize/2.0 > circle_max_y) {
        circle->posY = circle_max_y - circleSize/2.0;
    } else if (circle->posY - circleSize/2.0 < 0.0) {
        circle->posY = circleSize/2.0;
    }
}


void checkCollisions(struct Cell* cell) {
    if (!cell->isLeaf) {
        for (int i = 0; i < 4; i++)
            checkCollisions(&cell->subcells[i]);
        return;
    }

    for (int i = 0; i < cell->numCirclesInCell - 1; i++) {
        struct Circle* circle1 = &cell->circles[i];

        for (int j = i + 1; j < cell->numCirclesInCell; j++) {
            struct Circle* circle2 = &cell->circles[j];

            if (fabs(circle1->posX - circle2->posX) > circleSize ||
                fabs(circle1->posY - circle2->posY) > circleSize)
                continue;

            double dx = circle2->posX - circle1->posX;
            double dy = circle2->posY - circle1->posY;
            double distSquared = dx * dx + dy * dy;
            double sum_r = circleSize;

            if (distSquared < sum_r * sum_r) {
                if (distSquared < sum_r * sum_r) {
                    double dist = sqrt(distSquared);
                    double overlap = (sum_r - dist) / 2.0;
                    dx /= dist;
                    dy /= dist;

                    circle1->posX -= overlap * dx;
                    circle1->posY -= overlap * dy;
                    circle2->posX += overlap * dx;
                    circle2->posY += overlap * dy;

                    double dvx = circle2->velX - circle1->velX;
                    double dvy = circle2->velY - circle1->velY;
                    double dot = dvx * dx + dvy * dy;

                    circle1->velX += dot * dx;
                    circle1->velY += dot * dy;
                    circle2->velX -= dot * dx;
                    circle2->velY -= dot * dy;

                    circle1->velX *= friction;
                    circle1->velY *= friction;
                    circle2->velX *= friction;
                    circle2->velY *= friction;
                }
            }
        }
    }

    for (int i = 0; i < cell->numCirclesInCell; i++) {
        checkPosition(&cell->circles[i]);
    }
}

bool cellContainsCircle(struct Cell* cell, struct Circle* circle) {
    if (cell->isLeaf) {
        for (int i = 0; i < cell->numCirclesInCell; i++)
            if (circle == &cell->circles[i])
                return true;
    } else {
        for (int i = 0; i < 4; i++) {
            if (isCircleCloseToCellArea(circle, &cell->subcells[i]))
                if (cellContainsCircle(&cell->subcells[i], circle))
                    return true;
        }
    }
    return false;
}

bool isCircleFullInsideCellArea(struct Circle* circle, struct Cell* cell) {
    return circle->posX - circleSize / 2.0 >= cell->posX && circle->posX + circleSize / 2.0 <= cell->posX + cell->cellWidth && circle->posY - circleSize / 2.0 >= cell->posY && circle->posY + circleSize / 2.0 <= cell->posY + cell->cellHeight;
}

bool isCircleOverlappingCellArea(struct Circle* circle, struct Cell* cell) {
    return circle->posX + circleSize / 2.0 >= cell->posX && circle->posX - circleSize / 2.0 <= cell->posX + cell->cellWidth && circle->posY + circleSize / 2.0 >= cell->posY && circle->posY - circleSize / 2.0 <= cell->posY + cell->cellHeight;
}

bool isCircleCloseToCellArea(struct Circle* circle, struct Cell* cell) {
    return circle->posX + circleSize + maxSpeed >= cell->posX && circle->posX - circleSize - maxSpeed <= cell->posX + cell->cellWidth && circle->posY + circleSize + maxSpeed >= cell->posY && circle->posY - circleSize - maxSpeed <= cell->posY + cell->cellHeight;
}
bool isCircleOverlappingArea(struct Circle* circle, double cellPosX, double cellPosY, double cellWidth, double cellHeight) {
    return circle->posX + circleSize / 2.0 >= cellPosX && circle->posX - circleSize / 2.0 <= cellPosX + cellWidth && circle->posY + circleSize / 2.0 >= cellPosY && circle->posY - circleSize / 2.0 <= cellPosY + cellHeight;
}

void printTree(struct Cell* cell, int depth) {
    for (int i = 0; i < depth; i++) {
        printf("----");
    }
    printf("%d", cell->numCirclesInCell);

    if (cell->isLeaf) {
        printf(": ");
        for (int i = 0; i < cell->numCirclesInCell; i++) {
            printf("%d ", &cell->circles[i]);
        }
        printf("\n");
        return;
    }
    printf("\n");

    for (int i = 0; i < 4; i++)
        printTree(&cell->subcells[i], depth+1);
}

double random_double(double min, double max) {
    double range = max - min;
    double scaled = (double)rand() / RAND_MAX;  // random value between 0 and 1
    return min + (scaled * range);
}
