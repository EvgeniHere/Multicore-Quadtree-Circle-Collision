#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include "circle.c"
#include "hashset.c"

typedef struct Rectangle {
    double posX;
    double posY;
    double width;
    double height;
} Rectangle;

typedef struct Cell {
    double posX;
    double posY;
    double cellWidth;
    double cellHeight;
    int numCirclesInCell;
    Hashset* circles;
    int* circle_ids;
    struct Cell* subcells;
    struct Cell* parentCell;
} Cell;

typedef struct Process {
    int posX;
    int posY;
    int width;
    int height;
    int numCircles;
    int numCells;
    struct Circle* circles;
    struct Rectangle* rects;
} Process;

int numProcesses;
int numCircles;
int maxCirclesPerCell;
double minCellSize;

struct Cell* rootCell;
struct Circle* circles;
bool* circle_inside;
struct Process* processes;

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

int numCirclesInside = 0;

int* outgoingCircles;
struct Circle* ingoingCircles;
int numOutgoing = 0;
int maxOutgoing = 1000;
int numIngoing = 0;
int maxIngoing = 1000;

int numCells = 0;
struct Rectangle* leaf_rects;

bool* circle_inside;

void updateCell(struct Cell* cell);
bool addCircleToCell(int circle_id, struct Cell* cell);
void addCircleToParentCell(int circle_id, struct Cell* cell);
void split(struct Cell* cell);
void collapse(struct Cell* cell, struct Cell* originCell);
bool deleteCircle(struct Cell* cell, int circle_id);
void checkCollisions(struct Cell* cell);
bool cellContainsCircle(struct Cell* cell, int circle_id);
bool isCircleFullInsideCellArea(int circle_id, struct Cell* cell);
bool isCircleIDOverlappingCellArea(int circle_id, struct Cell* cell);
bool isCircleCloseToCellArea(int circle_id, struct Cell* cell);
bool isCircleOverlappingCellArea(struct Circle* circle, struct Cell* cell);
void printTree(struct Cell* cell, int depth);
double random_double(double min, double max);
void sendToDifferentProcess(int circle_id);
void* recvCircle(void* arg);
void* sendCircle(void* arg);

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
    rootCell->circle_ids = (int*)malloc(maxCirclesPerCell * sizeof(int));
    rootCell->circles = initializeHashSet();
    if (rootCell->circle_ids == NULL) {
        printf("Memory error!");
        exit(1);
    }
    rootCell->subcells = NULL;

    //leaf_rects = (struct Rectangle*)malloc(100 * sizeof(struct Rectangle));
    ingoingCircles = (struct Circle*)malloc(maxIngoing * sizeof(struct Circle));
    outgoingCircles = (int*)malloc(maxOutgoing * sizeof(int));
    circle_inside = (bool *) malloc(sizeof(bool) * numCircles);

    for (int i = 0; i < numCircles; i++) {
        circle_inside[i] = isCircleIDOverlappingCellArea(i, rootCell);
        if (!circle_inside[i])
            continue;
        addCircleToCell(i, rootCell);
        numCirclesInside++;
    }
    updateCell(rootCell);
}

int countCells(struct Cell* cell) {
    if (cell->circle_ids != NULL) {
        return 1;
    }
    int num = 0;
    for (int i = 0; i < 4; i++) {
        num += countCells(&cell->subcells[i]);
    }
    return num;
}

void updateRects(struct Cell* cell) {
    if (cell->circle_ids != NULL) {
        leaf_rects[numCells].posX = cell->posX;
        leaf_rects[numCells].posY = cell->posY;
        leaf_rects[numCells].width = cell->cellWidth;
        leaf_rects[numCells++].height = cell->cellHeight;
        return;
    }
    for (int i = 0; i < 4; i++) {
        updateRects(&cell->subcells[i]);
    }
}

void updateVisualsFromTree() {
    numCells = countCells(rootCell);
    leaf_rects = (struct Rectangle*) realloc(leaf_rects, numCells * sizeof(struct Rectangle));
    numCells = 0;
    updateRects(rootCell);
}

void updateCell(struct Cell* cell) {
    if (cell->circle_ids != NULL) {
        for (int i = 0; i < cell->numCirclesInCell; i++) {
            int circle_id = cell->circle_ids[i];
            if (isCircleFullInsideCellArea(circle_id, cell))
                continue;
            if (!isCircleIDOverlappingCellArea(circle_id, cell)) {
                deleteCircle(rootCell, circle_id);
                i--;
            }
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
    if (numIngoing > 0) {
        pthread_mutex_lock(&ingoingMutex);
        for (int i = 0; i < numIngoing; i++) {
            circles[ingoingCircles[i].id] = ingoingCircles[i];
            addCircleToCell(ingoingCircles[i].id, rootCell);
            if (!circle_inside[ingoingCircles[i].id]) {
                circle_inside[ingoingCircles[i].id] = true;
                numCirclesInside++;
            }
        }
        numIngoing = 0;
        pthread_mutex_unlock(&ingoingMutex);
    }
    for (int i = 0; i < numCircles; i++) {
        if (circle_inside[i]) {
            move(&circles[i]);
        }
        //deleteCircle(rootCell, i);
    }
    checkCollisions(rootCell);
    updateCell(rootCell);
}

bool addCircleToCell(int circle_id, struct Cell* cell) {
    if (cell->circle_ids != NULL) {
        if (cellContainsCircle(cell, circle_id))
            return true;

        if (cell->numCirclesInCell >= maxCirclesPerCell) {
            if (cell->cellWidth > minCellSize && cell->cellHeight > minCellSize) {
                split(cell);
                for (int i = 0; i < 4; i++) {
                    struct Cell* subcell = &cell->subcells[i];
                    if (!isCircleIDOverlappingCellArea(circle_id, subcell))
                        continue;
                    addCircleToCell(circle_id, subcell);
                }
                cell->numCirclesInCell++;
                return false;
            } else {
                cell->circle_ids = (int *) realloc(cell->circle_ids, ((cell->numCirclesInCell) + 1) * sizeof(int));
                if (cell->circle_ids == NULL) {
                    printf("Memory error!");
                    exit(1);
                }
            }
        }
        cell->circle_ids[cell->numCirclesInCell++] = circle_id;
        insert(cell->circles, circle_id);
        return false;
    }

    bool alreadyInsideCell = false;
    for (int i = 0; i < 4; i++) {
        struct Cell* subcell = &cell->subcells[i];
        if (!isCircleIDOverlappingCellArea(circle_id, subcell))
            continue;
        if (addCircleToCell(circle_id, subcell))
            alreadyInsideCell = true;
    }

    if (!alreadyInsideCell)
        cell->numCirclesInCell++;

    return alreadyInsideCell;
}

void sendToDifferentProcess(int circle_id) {
    if (numOutgoing >= maxOutgoing) {
        printf("Maximum outgoing circles reached!\n");
        while(numOutgoing >= maxOutgoing) {
            usleep(5000);
        }
    }

    pthread_mutex_lock(&outgoingMutex);

    for (int i = 0; i < numOutgoing; i++) {
        if (outgoingCircles[i] == circle_id) {
            pthread_mutex_unlock(&outgoingMutex);
            return;
        }
    }
    outgoingCircles[numOutgoing++] = circle_id;

    pthread_mutex_unlock(&outgoingMutex);
}

void addCircleToParentCell(int circle_id, struct Cell* cell) {
    if (cell == rootCell) {
        if (!isCircleOverlappingCellArea(&circles[circle_id], cell)) { // CHANGED THIS IF (REMOVE IF NECESSARY!!)
            circle_inside[circle_id] = false;
            numCirclesInside--;
            sendToDifferentProcess(circle_id);
        }
        return;
    }

    struct Cell* parentCell = cell->parentCell;

    if (isCircleIDOverlappingCellArea(circle_id, parentCell)) {
        for (int i = 0; i < 4; i++) {
            struct Cell* neighbourCell = &parentCell->subcells[i];
            if (neighbourCell == cell || !isCircleIDOverlappingCellArea(circle_id, neighbourCell))
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
        cell->subcells[i].numCirclesInCell = 0;
        cell->subcells[i].parentCell = cell;
        cell->subcells[i].subcells = NULL;
        cell->subcells[i].circle_ids = (int*)malloc(maxCirclesPerCell * sizeof(int));
        cell->subcells[i].circles = initializeHashSet();
        if (cell->subcells[i].circle_ids == NULL) {
            printf("Memory error!");
            exit(1);
        }
    }

    for (int i = 0; i < cell->numCirclesInCell; i++) {
        int circle_id = cell->circle_ids[i];
        for (int j = 0; j < 4; j++) {
            struct Cell* subcell = &cell->subcells[j];
            if (!isCircleIDOverlappingCellArea(circle_id, subcell))
                continue;
            addCircleToCell(circle_id, subcell);
        }
    }

    free(cell->circle_ids);
    cell->circle_ids = NULL;
    destroyHashset(cell->circles);
    cell->circles = NULL;
}

void collapse(struct Cell* cell, struct Cell* originCell) {
    if (cell == NULL) {
        return;
    }

    if (cell == originCell) {
        if (cell->circle_ids != NULL) {
            return;
        }
        cell->numCirclesInCell = 0;
        cell->circle_ids = (int *) malloc(maxCirclesPerCell * sizeof(int));
        cell->circles = initializeHashSet();
        if (cell->circle_ids == NULL) {
            printf("Error: Failed to allocate memory for circle ids in cell.\n");
            exit(1);
        }
    } else if (cell->circle_ids != NULL) {
        for (int i = 0; i < cell->numCirclesInCell; i++) {
            int circle_id = cell->circle_ids[i];
            if (!isCircleIDOverlappingCellArea(circle_id, originCell) || cellContainsCircle(originCell, circle_id))
                continue;
            if (originCell->numCirclesInCell > maxCirclesPerCell) {
                printf("WTF!\n");
                return;
            } else {
                originCell->circle_ids[originCell->numCirclesInCell++] = circle_id;
                insert(originCell->circles, circle_id);
            }
        }
        free(cell->circle_ids);
        cell->circle_ids = NULL;
        destroyHashset(cell->circles);
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

bool deleteCircle(struct Cell* cell, int circle_id) {
    if (cell->circle_ids != NULL) {
        if (isCircleIDOverlappingCellArea(circle_id, cell))
            return false;
        if (!cellContainsCircle(cell, circle_id))
            return false;
        for (int i = 0; i < cell->numCirclesInCell; i++) {
            if (cell->circle_ids[i] != circle_id)
                continue;
            for (int j = i; j < cell->numCirclesInCell - 1; j++) {
                cell->circle_ids[j] = cell->circle_ids[j + 1];
            }
            cell->numCirclesInCell--;
            removeValue(cell->circles, circle_id);
            return true;
        }
    } else {
        if (isCircleCloseToCellArea(circle_id, cell)) {
            bool deleted = false;
            for (int i = 0; i < 4; i++) {
                if (deleteCircle(&cell->subcells[i], circle_id))
                    deleted = true;
            }
            if (deleted && !isCircleIDOverlappingCellArea(circle_id, cell)) {
                cell->numCirclesInCell--;
                return true;
            }
        }
    }
    return false;
}

void checkCollisions(struct Cell* cell) {
    if (cell->circle_ids == NULL) {
        for (int i = 0; i < 4; i++)
            checkCollisions(&cell->subcells[i]);
        return;
    }

    for (int i = 0; i < cell->numCirclesInCell; i++) {
        int id_1 = cell->circle_ids[i];
        struct Circle* circle1 = &circles[id_1];

        for (int j = i + 1; j < cell->numCirclesInCell; j++) {
            int id_2 = cell->circle_ids[j];

            struct Circle* circle2 = &circles[id_2];

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

        checkPosition(circle1);
    }
}

bool cellContainsCircle(struct Cell* cell, int circle_id) {
    if (cell->circles != NULL) {
        return contains(cell->circles, circle_id);
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

bool isCircleIDOverlappingCellArea(int circle_id, struct Cell* cell) {
    return circles[circle_id].posX + circleSize / 2.0 > cell->posX && circles[circle_id].posX - circleSize / 2.0 < cell->posX + cell->cellWidth && circles[circle_id].posY + circleSize / 2.0 > cell->posY && circles[circle_id].posY - circleSize / 2.0 < cell->posY + cell->cellHeight;
}

bool isCircleCloseToCellArea(int circle_id, struct Cell* cell) {
    return circles[circle_id].posX + minCellSize > cell->posX && circles[circle_id].posX - minCellSize < cell->posX + cell->cellWidth && circles[circle_id].posY + minCellSize > cell->posY && circles[circle_id].posY - minCellSize < cell->posY + cell->cellHeight;
}

bool isCircleOverlappingArea(struct Circle* circle, double posX, double posY, double width, double height) {
    return circle->posX + circleSize / 2.0 > posX && circle->posX - circleSize / 2.0 < posX + width && circle->posY + circleSize / 2.0 > posY && circle->posY - circleSize / 2.0 < posY + height;
}

bool isCircleOverlappingCellArea(struct Circle* circle, struct Cell* cell) {
    return circle->posX + circleSize / 2.0 >= cell->posX && circle->posX - circleSize / 2.0 <= cell->posX + cell->cellWidth && circle->posY + circleSize / 2.0 >= cell->posY && circle->posY - circleSize / 2.0 <= cell->posY + cell->cellHeight;
}

void printTree(struct Cell* cell, int depth) {
    for (int i = 0; i < depth; i++) {
        printf("----");
    }
    printf("%d", cell->numCirclesInCell);

    if (cell->circle_ids != NULL) {
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
        if (!alreadyInside && !circle_inside[circle->id]) {
            ingoingCircles[numIngoing++] = *circle;
        }
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
                if (!isCircleOverlappingArea(&circles[outgoingCircles[i]], processes[j].posX, processes[j].posY, processes[j].width, processes[j].height))
                    continue;
                MPI_Isend(&circles[outgoingCircles[i]], sizeof(struct Circle), MPI_BYTE, j, tag_circle, MPI_COMM_WORLD, &requests[i]);
                num++;
                break;
            }
        }

        // VIELLEICHT BESSER FÜR JEDEN PROZESS EIN ARRAY DANN GANZEN ARRAY ABSCHICKEN?

        if (numOutgoing != num) {
            printf("%d WRONG NUMBER %d %d\n", outgoingCircles[0], numOutgoing, num);
        }

        numOutgoing = 0;
        pthread_mutex_unlock(&outgoingMutex);

        MPI_Waitall(oldNumOutgoing, requests, statuses);

        free(requests);
        free(statuses);
    }
    return NULL;
}
