#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include "circle.c"
//#include "hashset.c"

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
    //struct Hashset circles;
    struct Circle* circles;
    struct Cell* subcells;
    struct Cell* parentCell;
};

struct Process {
    int posX;
    int posY;
    int width;
    int height;
    int numCircles;
    int numCells;
    struct Circle* circles;
    struct Rectangle* cells;
};

int numProcesses;
struct Process* processes;

int maxCirclesPerCell = 3;
double minCellSize = 10.0;

struct Cell* rootCell;

int numCircles = 0;
struct Circle* circles;

int tag_circle = 0;
int tag_circles = 1;
int tag_numCircles = 2;
int tag_process = 3;
int tag_numCells = 4;
int tag_cells = 5;
int tag_circle_inside = 6;
int last_tag = 7;

pthread_mutex_t outgoingMutex;
pthread_mutex_t ingoingMutex;

int rank, size;

struct Circle* outgoingCircles;
struct Circle* ingoingCircles;
int numOutgoing = 0;
int maxOutgoing = 1000;
int numIngoing = 0;
int maxIngoing = 1000;

int numRects = 0;
struct Rectangle* rects;

void updateCell(struct Cell* cell);
bool addCircleToCell(struct Circle* circle, struct Cell* cell);
void addCircleToParentCell(struct Circle* circle, struct Cell* cell);
void split(struct Cell* cell);
void collapse(struct Cell* cell, struct Cell* originCell);
bool deleteCircle(struct Cell* cell, struct Circle* circle);
void checkCollisions(struct Cell* cell);
bool cellContainsCircle(struct Cell* cell, struct Circle* circle);
bool isCircleFullInsideCellArea(struct Circle* circle, struct Cell* cell);
bool isCircleOverlappingCellArea(struct Circle* circle, struct Cell* cell);
bool isCircleOverlappingArea(struct Circle* circle, double posX, double posY, double width, double height);
bool isCircleCloseToCellArea(struct Circle* circle, struct Cell* cell);
bool isCircleFullInsideArea(struct Circle* circle, double posX, double posY, double width, double height);
void printTree(struct Cell* cell, int depth);
double random_double(double min, double max);
void updateVisualsFromTree();

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
    rootCell->numCirclesInCell = 0;
    rootCell->parentCell = NULL;
    rootCell->circles = (struct Circle*)malloc(maxCirclesPerCell * sizeof(struct Circle));
    if (rootCell->circles == NULL) {
        printf("Memory error!");
        exit(1);
    }
    rootCell->subcells = NULL;

    outgoingCircles = (struct Circle*) malloc(maxOutgoing * sizeof(struct Circle));
    ingoingCircles = (struct Circle*) malloc(maxIngoing * sizeof(struct Circle));
    rects = (struct Rectangle*) malloc(sizeof(struct Rectangle));

    for (int i = 0; i < numCircles; i++) {
        addCircleToCell(&circles[i], rootCell);
    }
    updateCell(rootCell);
    //checkCollisions(rootCell);
    //printTree(rootCell, 0);
}

int countCells(struct Cell* cell) {
    if (cell->circles != NULL) {
        return 1;
    }

    int num = 0;
    for (int i = 0; i < 4; i++) {
        num += countCells(&cell->subcells[i]);
    }
    return num;
}

void updateRects(struct Cell* cell) {
    if (cell->circles != NULL) {
        rects[numRects].posX = cell->posX;
        rects[numRects].posY = cell->posY;
        rects[numRects].width = cell->cellWidth;
        rects[numRects++].height = cell->cellHeight;
        return;
    }

    for (int i = 0; i < 4; i++) {
        updateRects(&cell->subcells[i]);
    }
}

int countCircles(struct Cell* cell) {
    if (cell->circles != NULL) {
        return cell->numCirclesInCell;
    }
    if (cell->subcells == NULL)
        return 0;
    int numCellCircles = 0;
    for (int i = 0; i < 4; i++) {
        numCellCircles += countCircles(&cell->subcells[i]);
    }
    return numCellCircles;
}

void updateCirclesFromCell(struct Cell* cell) {
    if (cell->circles != NULL) {
        for (int i = 0; i < cell->numCirclesInCell; i++) {
            circles[numCircles++] = cell->circles[i];
        }
    } else if (cell->subcells != NULL) {
        for (int i = 0; i < 4; i++) {
            updateCirclesFromCell(&cell->subcells[i]);
        }
    }
}

void updateVisualsFromTree() {
    numCircles = countCircles(rootCell);
    circles = (struct Circle*) realloc(circles, numCircles * sizeof(struct Circle));
    numCircles = 0;
    updateCirclesFromCell(rootCell);
    /*numRects = countCells(rootCell);
    rects = (struct Rectangle*) realloc(rects, numRects * sizeof(struct Rectangle));
    numRects = 0;
    updateRects(rootCell);*/
}

void updateTree() {
    if (numIngoing > 0) {
        pthread_mutex_lock(&ingoingMutex);
        for (int i = 0; i < numIngoing; i++) {
            addCircleToCell(&ingoingCircles[i], rootCell);
        }
        numIngoing = 0;
        pthread_mutex_unlock(&ingoingMutex);
    }

    updateCell(rootCell);

    checkCollisions(rootCell);
}

// Return: cellContainsCell
bool addCircleToCell(struct Circle* circle, struct Cell* cell) {
    if (cell->circles != NULL) {
        if (cellContainsCircle(cell, circle)) {
            return true;
        }

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
                return false;
            } else {
                cell->circles = (struct Circle *) realloc(cell->circles, (cell->numCirclesInCell + 1) * sizeof(struct Circle));
                if (cell->circles == NULL) {
                    printf("Memory error!");
                    exit(1);
                }
            }
        }
        cell->circles[cell->numCirclesInCell++] = *circle;
        return false;
    }
    if (cell->subcells == NULL)
        return true;

    bool alreadyInsideCell = false;
    for (int i = 0; i < 4; i++) {
        struct Cell* subcell = &cell->subcells[i];
        if (!isCircleOverlappingCellArea(circle, subcell))
            continue;
        if (addCircleToCell(circle, subcell)) {
            alreadyInsideCell = true;
        }
    }

    if (!alreadyInsideCell && isCircleOverlappingCellArea(circle, cell)) //2nd check is useless
        cell->numCirclesInCell++;

    return alreadyInsideCell;
}

void sendToDifferentProcess(struct Circle* circle) {
    if (numOutgoing >= maxOutgoing) {
        printf("Maximum outgoing circles reached!\n");
        while(numOutgoing >= maxOutgoing) {
            usleep(5000);
        }
    }

    pthread_mutex_lock(&outgoingMutex);

    for (int i = 0; i < numOutgoing; i++) {
        if (outgoingCircles[i].id == circle->id) {
            pthread_mutex_unlock(&outgoingMutex);
            return;
        }
    }
    outgoingCircles[numOutgoing++] = *circle;

    pthread_mutex_unlock(&outgoingMutex);
}

void addCircleToParentCell(struct Circle* circle, struct Cell* cell) {
    if (cell == rootCell) {
        if (!isCircleOverlappingCellArea(circle, rootCell)) {
            sendToDifferentProcess(circle);
        }
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

    free(cell->circles);
    cell->circles = NULL;
}

void collapse(struct Cell* cell, struct Cell* originCell) {
    if (cell == NULL) {
        return;
    }

    if (cell == originCell) {
        if (cell->circles != NULL) {
            return;
        }
        cell->numCirclesInCell = 0;
        cell->circles = (struct Circle *) malloc(maxCirclesPerCell * sizeof(struct Circle));
        if (cell->circles == NULL) {
            printf("Error: Failed to allocate memory for circle ids in cell.\n");
            exit(1);
        }
    } else if (cell->circles != NULL) {
        for (int i = 0; i < cell->numCirclesInCell; i++) {
            struct Circle* circle = &cell->circles[i];
            if (cellContainsCircle(originCell, circle))
                continue;
            if (originCell->numCirclesInCell > maxCirclesPerCell) {
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
}

bool deleteCircle(struct Cell* cell, struct Circle* circle) {
    if (cell->circles != NULL) {
        if (isCircleOverlappingCellArea(circle, cell))
            return false;
        for (int i = 0; i < cell->numCirclesInCell; i++) {
            if (cell->circles[i].id != circle->id)
                continue;
            for (int j = i; j < cell->numCirclesInCell - 1; j++) {
                cell->circles[j] = cell->circles[j + 1];
            }
            cell->numCirclesInCell--;
            if (cell->numCirclesInCell >= maxCirclesPerCell) {
                cell->circles = (struct Circle *) realloc(cell->circles, cell->numCirclesInCell * sizeof(struct Circle));
            }
            return true;
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

void updateCell(struct Cell* cell) {
    if (cell->circles != NULL) {
        for (int i = 0; i < cell->numCirclesInCell; i++) {
            struct Circle* circle = &cell->circles[i];
            if (!isCircleFullInsideCellArea(circle, cell)) {
                addCircleToParentCell(circle, cell);
                if (cell->subcells != NULL) {
                    updateCell(cell);
                    return;
                }
            }
            if (!isCircleOverlappingCellArea(circle, cell)) {
                deleteCircle(rootCell, circle);
                i--;
            }
        }
    } else if (cell->subcells != NULL) {
        for (int i = 0; i < 4; i++) {
            updateCell(&cell->subcells[i]);
        }
        if (cell->numCirclesInCell <= maxCirclesPerCell)
            collapse(cell, cell);
    }
}

void checkCollisions(struct Cell* cell) {
    if (cell->subcells != NULL) {
        for (int i = 0; i < 4; i++) {
            checkCollisions(&cell->subcells[i]);
        }
        return;
    }

    for (int i = 0; i < cell->numCirclesInCell; i++) {
        struct Circle* circle1 = &cell->circles[i];

        for (int j = i + 1; j < cell->numCirclesInCell; j++) {
            struct Circle* circle2 = &cell->circles[j];

            if (circle1->id == circle2->id)
                continue;

            double dx = circle2->posX - circle1->posX;
            double dy = circle2->posY - circle1->posY;

            if (fabs(dx) > circleSize || fabs(dy) > circleSize)
                continue;

            double distSquared = dx * dx + dy * dy;

            if (distSquared < circleSize * circleSize) {
                double dist = sqrt(distSquared);
                double overlap = (circleSize - dist) / 2.0;
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

        move(circle1);
    }
}

bool cellContainsCircle(struct Cell* cell, struct Circle* circle) {
    if (cell->circles != NULL) {
        for (int i = 0; i < cell->numCirclesInCell; i++)
            if (circle->id == cell->circles[i].id)
                return true;
    } else if (cell->subcells != NULL) {
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
    return circle->posX + circleSize / 2.0 > cell->posX && circle->posX - circleSize / 2.0 < cell->posX + cell->cellWidth && circle->posY + circleSize / 2.0 > cell->posY && circle->posY - circleSize / 2.0 < cell->posY + cell->cellHeight;
}

bool isCircleCloseToCellArea(struct Circle* circle, struct Cell* cell) {
    return circle->posX + 2 * circleSize + 4 * maxSpeed >= cell->posX && circle->posX - 2 * circleSize - 4 * maxSpeed <= cell->posX + cell->cellWidth && circle->posY + 2 * circleSize +  4 * maxSpeed >= cell->posY && circle->posY - 2 * circleSize - 4 * maxSpeed <= cell->posY + cell->cellHeight;
}

bool isCircleOverlappingArea(struct Circle* circle, double posX, double posY, double width, double height) {
    return circle->posX + circleSize / 2.0 > posX && circle->posX - circleSize / 2.0 < posX + width && circle->posY + circleSize / 2.0 > posY && circle->posY - circleSize / 2.0 < posY + height;
}

bool isCircleFullInsideArea(struct Circle* circle, double posX, double posY, double width, double height) {
    return circle->posX - circleSize / 2.0 >= posX && circle->posX + circleSize / 2.0 <= posX + width && circle->posY - circleSize / 2.0 >= posY && circle->posY + circleSize / 2.0 <= posY + height;
}

void printTree(struct Cell* cell, int depth) {
    for (int i = 0; i < depth; i++) {
        printf("----");
    }
    printf("%d", cell->numCirclesInCell);

    if (cell->circles != NULL) {
        printf(": ");
        for (int i = 0; i < cell->numCirclesInCell; i++) {
            printf("%d ", cell->circles[i].id);
        }
        printf("\n");
        return;
    }
    printf("\n");

    if (cell->subcells == NULL)
        return;
    for (int i = 0; i < 4; i++)
        printTree(&cell->subcells[i], depth+1);
}

double random_double(double min, double max) {
    double range = max - min;
    double scaled = (double)rand() / RAND_MAX;  // random value between 0 and 1
    return min + (scaled * range);
}

void* receiveCircle(void* arg) {
    struct Circle* circle = (struct Circle*) malloc(sizeof(struct Circle));

    while (true) {
        MPI_Request request;
        MPI_Status status;

        MPI_Irecv(circle, sizeof(struct Circle), MPI_BYTE, MPI_ANY_SOURCE, tag_circle, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, &status);

        pthread_mutex_lock(&ingoingMutex);
        bool alreadyInside = false;
        for (int i = 0; i < numIngoing; i++) {
            if (ingoingCircles[i].id == circle->id) {
                alreadyInside = true;
                break;
            }
        }
        if (!alreadyInside)
            ingoingCircles[numIngoing++] = *circle;
        pthread_mutex_unlock(&ingoingMutex);
    }

    free(circle);
    return NULL;
}

void* sendCircle(void* arg) {
    int oldNumOutgoing;

    while (true) {
        while (numOutgoing == 0) {
            usleep(5000);
        }

        pthread_mutex_lock(&outgoingMutex);
        oldNumOutgoing = numOutgoing;

        MPI_Request* requests = malloc(numOutgoing * sizeof(MPI_Request));
        MPI_Status* statuses = malloc(numOutgoing * sizeof(MPI_Status));

        int num = 0;
        for (int i = 0; i < numOutgoing; i++) {
            for (int j = 0; j < numProcesses; j++) {
                if (j == rank)
                    continue;
                if (!isCircleOverlappingArea(&outgoingCircles[i], processes[j].posX, processes[j].posY, processes[j].width, processes[j].height))
                    continue;
                MPI_Isend(&outgoingCircles[i], sizeof(struct Circle), MPI_BYTE, j, tag_circle, MPI_COMM_WORLD, &requests[i]);
                num++;
                break;
            }
        }

        if (numOutgoing != num)
            printf("%d %d\n", numOutgoing, num);

        numOutgoing = 0;
        MPI_Waitall(oldNumOutgoing, requests, statuses);

        pthread_mutex_unlock(&outgoingMutex);

        free(requests);
        free(statuses);
    }
    return NULL;
}
