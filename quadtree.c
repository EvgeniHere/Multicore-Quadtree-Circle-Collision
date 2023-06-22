#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "circle.c"

struct Rectangle {
    double posX;
    double posY;
    double width;
    double height;
};

struct Cell {
    double posX;
    double posY;
    double cellWidth;
    double cellHeight;
    int numCirclesInCell;
    bool isLeaf;
    int* circle_ids;
    struct Cell* subcells;
};

int maxCirclesPerCell;
double minCellSize;

struct Cell* rootCell;
struct Rectangle* leaf_rects;

int numCells = 0;

int numCircles;
struct Circle* circles;
void rebuildTree();
void freeTree(struct Cell* cell);
void addCircleToCell(int circle_id, struct Cell* cell);
void split(struct Cell* cell);
void checkCollision(int circle_id, struct Cell* cell);
bool cellContainsCircle(struct Cell* cell, int circle_id);
bool isCircleOverlappingCellArea(int circle_id, struct Cell* cell);
bool isCircleCloseToCellArea(int circle_id, struct Cell* cell);
bool isCircleOverlappingArea(struct Circle* circle, double cellPosX, double cellPosY, double cellWidth, double cellHeight);
void printTree(struct Cell* cell, int depth);
double random_double(double min, double max);

int treeWidth;
int treeHeight;
int treePosX;
int treePosY;

int countLeafCells(struct Cell* cell) {
    if (cell->isLeaf) {
        return 1;
    }

    int numLeafs = 0;
    for (int i = 0; i < 4; i++) {
        numLeafs += countLeafCells(&cell->subcells[i]);
    }
    return numLeafs;
}

void calcLeafRectangles(struct Cell* cell) {
    if (cell->isLeaf) {
        struct Rectangle* rect = (struct Rectangle*) malloc(sizeof(struct Rectangle));
        rect->posX = cell->posX;
        rect->posY = cell->posY;
        rect->width = cell->cellWidth;
        rect->height = cell->cellHeight;
        leaf_rects[numCells++] = *rect;
        return;
    }
    for (int i = 0; i < 4; i++) {
        calcLeafRectangles(&cell->subcells[i]);
    }
}

void rebuildTree() {
    rootCell = (struct Cell*)malloc(sizeof(struct Cell));
    if (rootCell == NULL) {
        printf("Memory error!");
        exit(1);
    }
    rootCell->posX = treePosX;
    rootCell->posY = treePosY;
    rootCell->cellWidth = treeWidth;
    rootCell->cellHeight = treeHeight;
    rootCell->isLeaf = true;
    rootCell->numCirclesInCell = 0;
    rootCell->circle_ids = (int*)malloc(maxCirclesPerCell * sizeof(int));
    if (rootCell->circle_ids == NULL) {
        printf("Memory error!");
        exit(1);
    }
    rootCell->subcells = NULL;
    for (int i = 0; i < numCircles; i++) {
        addCircleToCell(i, rootCell);
        move(&circles[i]);
    }
    numCells = countLeafCells(rootCell);
    leaf_rects = (struct Rectangle*) malloc(numCells * sizeof(struct Rectangle));
    numCells = 0;
    calcLeafRectangles(rootCell);
}

void freeTree(struct Cell* cell) {
    if (cell->isLeaf) {
        free(cell->circle_ids);
        cell->numCirclesInCell = 0;
    } else {
        for (int i = 0; i < 4; i++) {
            freeTree(&cell->subcells[i]);
        }
        free(cell->subcells);
        cell->isLeaf = true;
    }
    if (cell == rootCell)
        free(rootCell);
}

void addCircleToCell(int circle_id, struct Cell* cell) {
    if (!isCircleOverlappingCellArea(circle_id, cell))
        return;

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
                return;
            } else {
                cell->circle_ids = (int *) realloc(cell->circle_ids, ((cell->numCirclesInCell) + 1) * sizeof(int));
                if (cell->circle_ids == NULL) {
                    printf("Memory error!");
                    exit(1);
                }
            }
        }
        checkCollision(circle_id, cell);
        cell->circle_ids[cell->numCirclesInCell++] = circle_id;
        return;
    }

    for (int i = 0; i < 4; i++) {
        struct Cell* subcell = &cell->subcells[i];
        if (!isCircleOverlappingCellArea(circle_id, subcell))
            continue;
        addCircleToCell(circle_id, subcell);
    }
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
        cell->subcells[i].subcells = NULL;
        cell->subcells[i].circle_ids = (int*)malloc(maxCirclesPerCell * sizeof(int));
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
    cell->circle_ids = NULL;
    cell->numCirclesInCell = 0;
    free(cell->circle_ids);
}

void checkCollision(int circle_id, struct Cell* cell) {
    int id_1 = circle_id;

    for (int j = 0; j < cell->numCirclesInCell; j++) {
        if (id_1 == j)
            continue;
        int id_2 = cell->circle_ids[j];

        if (fabs(circles[id_1].posX - circles[id_2].posX) > circleSize ||
            fabs(circles[id_1].posY - circles[id_2].posY) > circleSize)
            continue;

        double dx = circles[id_2].posX - circles[id_1].posX;
        double dy = circles[id_2].posY - circles[id_1].posY;
        double distSquared = dx * dx + dy * dy;
        double sum_r = circleSize;

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
bool isCircleCloseToCellArea(int circle_id, struct Cell* cell) {
    return circles[circle_id].posX + circleSize + maxSpeed >= cell->posX && circles[circle_id].posX - circleSize - maxSpeed <= cell->posX + cell->cellWidth && circles[circle_id].posY + circleSize + maxSpeed >= cell->posY && circles[circle_id].posY - circleSize - maxSpeed <= cell->posY + cell->cellHeight;
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
            printf("%d ", cell->circle_ids[i]);
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
