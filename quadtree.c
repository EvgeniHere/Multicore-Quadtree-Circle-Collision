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
    struct Cell* parentCell;
};

int maxCirclesPerCell = 3;
double minCellSize = 10.0;

struct Cell* rootCell;

int numCircles = 0;
struct Circle* circles;

bool* circle_inside;

void updateCell(struct Cell* cell);
void addCircleToCell(int circle_id, struct Cell* cell);
void addCircleToParentCell(int circle_id, struct Cell* cell);
void split(struct Cell* cell);
void collapse(struct Cell* cell, struct Cell* originCell);
bool deleteCircle(struct Cell* cell, int circle_id);
void checkCollisions(struct Cell* cell);
bool cellContainsCircle(struct Cell* cell, int circle_id);
bool isCircleFullInsideCellArea(int circle_id, struct Cell* cell);
bool isCircleOverlappingCellArea(int circle_id, struct Cell* cell);
bool isCircleCloseToCellArea(int circle_id, struct Cell* cell);
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
    rootCell->circle_ids = (int*)malloc(maxCirclesPerCell * sizeof(int));
    if (rootCell->circle_ids == NULL) {
        printf("Memory error!");
        exit(1);
    }
    rootCell->subcells = NULL;

    for (int i = 0; i < numCircles; i++) {
        circle_inside[i] = isCircleOverlappingCellArea(i, rootCell);
        if (circle_inside[i])
            addCircleToCell(i, rootCell);
    }
    updateCell(rootCell);
}

void updateCell(struct Cell* cell) {
    if (cell->isLeaf) {
        for (int i = 0; i < cell->numCirclesInCell; i++) {
            int circle_id = cell->circle_ids[i];
            if (isCircleFullInsideCellArea(circle_id, cell))
                continue;
            /*if (!isCircleOverlappingCellArea(circle_id, cell)) {
                deleteCircle(rootCell, circle_id);
                i--;
            }*/
            addCircleToParentCell(circle_id, cell);
        }
        return;
    }

    for (int i = 0; i < 4; i++) {
        updateCell(&cell->subcells[i]);
    }
    if (cell->numCirclesInCell <= maxCirclesPerCell)
        collapse(cell, cell);
}

void updateTree() {
    for (int i = 0; i < numCircles; i++) {
        //if (circle_inside[i])
        //deleteCircle(rootCell, i);
        if (isCircleOverlappingCellArea(i, rootCell)) {
            if (!circle_inside[i]) {
                addCircleToCell(i, rootCell);
                circle_inside[i] = true;
            }
            move(&circles[i]);
        }
        deleteCircle(rootCell, i);
    }
    checkCollisions(rootCell);
    updateCell(rootCell);
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
    if (cell == rootCell) {
        circle_inside[circle_id] = false;
        return;
    }

    struct Cell* parentCell = cell->parentCell;

    if (isCircleOverlappingCellArea(circle_id, parentCell)) {
        for (int i = 0; i < 4; i++) {
            struct Cell* neighbourCell = &parentCell->subcells[i];
            if (neighbourCell == cell || !isCircleOverlappingCellArea(circle_id, neighbourCell))
                continue;
            addCircleToCell(circle_id, neighbourCell);
        }
        if (isCircleFullInsideCellArea(circle_id, parentCell))
            return;
    }

    addCircleToParentCell(circle_id, parentCell);
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
                printTree(originCell, 0);
                for(int j = 0; j < 4; j++) {
                    printTree(&originCell->subcells[j], 0);
                }
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

bool deleteCircle(struct Cell* cell, int circle_id) {
    if (cell->isLeaf) {
        if (isCircleOverlappingCellArea(circle_id, cell))
            return false;
        for (int i = 0; i < cell->numCirclesInCell; i++) {
            if (cell->circle_ids[i] != circle_id)
                continue;
            for (int j = i; j < cell->numCirclesInCell - 1; j++) {
                cell->circle_ids[j] = cell->circle_ids[j + 1];
            }
            cell->numCirclesInCell--;
            return true;
        }
    } else {
        bool deleted = false;
        for (int i = 0; i < 4; i++) {
            if (isCircleCloseToCellArea(circle_id, &cell->subcells[i]))
                if (deleteCircle(&cell->subcells[i], circle_id))
                    deleted = true;
        }
        if (deleted && !isCircleOverlappingCellArea(circle_id, cell)) {
            cell->numCirclesInCell--;
            return true;
        }
    }
    return false;
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
    }

    for (int i = 0; i < cell->numCirclesInCell; i++) {
        int id = cell->circle_ids[i];
        checkPosition(&circles[id]);
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

bool isCircleFullInsideCellArea(int circle_id, struct Cell* cell) {
    return circles[circle_id].posX - circleSize / 2.0 >= cell->posX && circles[circle_id].posX + circleSize / 2.0 <= cell->posX + cell->cellWidth && circles[circle_id].posY - circleSize / 2.0 >= cell->posY && circles[circle_id].posY + circleSize / 2.0 <= cell->posY + cell->cellHeight;
}

bool isCircleOverlappingCellArea(int circle_id, struct Cell* cell) {
    return circles[circle_id].posX + circleSize / 2.0 >= cell->posX && circles[circle_id].posX - circleSize / 2.0 <= cell->posX + cell->cellWidth && circles[circle_id].posY + circleSize / 2.0 >= cell->posY && circles[circle_id].posY - circleSize / 2.0 <= cell->posY + cell->cellHeight;
}

bool isCircleCloseToCellArea(int circle_id, struct Cell* cell) {
    return circles[circle_id].posX + circleSize + maxSpeed >= cell->posX && circles[circle_id].posX - circleSize - maxSpeed <= cell->posX + cell->cellWidth && circles[circle_id].posY + circleSize + maxSpeed >= cell->posY && circles[circle_id].posY - circleSize - maxSpeed <= cell->posY + cell->cellHeight;
}

bool isCircleOverlappingArea(struct Circle* circle, double posX, double posY, double width, double height) {
    return circle->posX + circleSize / 2.0 >= posX && circle->posX - circleSize / 2.0 <= posX + width && circle->posY + circleSize / 2.0 >= posY && circle->posY - circleSize / 2.0 <= posY + height;
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
