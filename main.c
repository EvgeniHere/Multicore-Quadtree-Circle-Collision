#include <stdlib.h>
#include <mpi.h>
#include "window.c"
#include "quadtree.c"
#include <stdio.h>
#include <time.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 1000

int tag_circles = 1;
int tag_numCircles = 2;
int tag_process = 3;

int frames = 0;
clock_t begin;

int rank, size;

int numProcesses;

int numAllCircles;
struct Circle* allCircles;

int main(int argc, char** argv);
void update();
void distributeCircles();

struct Process {
    int posX;
    int posY;
    int width;
    int height;
    int numCircles;
    int* circle_ids;
    struct Circle* circles;
};

struct Process* processes;
struct Process* curProcess;

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 1) {
        fprintf(stderr, "This program requires at least 1 process.\n");
        MPI_Finalize();
        return 1;
    }

    numProcesses = size;
    numAllCircles = 100000;
    circleSize = 1.0;
    maxSpeed = 1.0;
    maxCirclesPerCell = 20;
    minCellSize = 2 * circleSize + 4 * maxSpeed;
    circle_max_X = SCREEN_WIDTH;
    circle_max_y = SCREEN_HEIGHT;

    numCircles = numAllCircles/numProcesses;
    circles = (struct Circle *) malloc(sizeof(struct Circle) * numCircles);
    allCircles = (struct Circle *) malloc(sizeof(struct Circle) * numAllCircles);

    if (rank == 0) {
        srand(90);
        for (int i = 0; i < numAllCircles; i++) {
            allCircles[i].posX = random_double(circleSize / 2.0, SCREEN_WIDTH - circleSize / 2.0);
            allCircles[i].posY = random_double(circleSize / 2.0, SCREEN_HEIGHT - circleSize / 2.0);
            allCircles[i].velX = random_double(-maxSpeed, maxSpeed);
            allCircles[i].velY = random_double(-maxSpeed, maxSpeed);
        }

        processes = (struct Process*) malloc(numProcesses * sizeof(struct Process));

        int numColumns = (int)ceil(sqrt(numProcesses));
        int numRows = (int)ceil((float)numProcesses / numColumns);

        int rectWidth = SCREEN_WIDTH / numColumns;
        int rectHeight = SCREEN_HEIGHT / numRows;

        int rectIndex = 0;
        for (int y = 0; y < numRows; y++) {
            for (int x = 0; x < numColumns; x++) {
                if (rectIndex >= numProcesses) {
                    break;
                }
                if (y == numRows - 1 && x == 0 && rectIndex + numColumns >= numProcesses) {
                    rectWidth = SCREEN_WIDTH / (numProcesses - rectIndex);
                }
                processes[rectIndex].posX = x * rectWidth;
                processes[rectIndex].posY = y * rectHeight;
                processes[rectIndex].width = rectWidth;
                processes[rectIndex].height = rectHeight;
                rectIndex++;
            }
        }

        for (int i = 0; i < numProcesses; i++) {
            processes[i].circles = (struct Circle *) malloc(numCircles * sizeof(struct Circle));
            processes[i].circle_ids = (int *) malloc(processes[i].numCircles * sizeof(int));
            processes[i].numCircles = 0;
            if (i > 0)
                MPI_Send(&processes[i], sizeof(struct Process), MPI_BYTE, i, tag_process, MPI_COMM_WORLD);
        }

        treePosX = processes[0].posX;
        treePosY = processes[0].posY;
        treeWidth = processes[0].width;
        treeHeight = processes[0].height;

        distributeCircles();
    } else {
        curProcess = (struct Process*) malloc(sizeof(struct Process));
        MPI_Recv(curProcess, sizeof(struct Process), MPI_BYTE, 0, tag_process, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        treePosX = curProcess->posX;
        treePosY = curProcess->posY;
        treeWidth = curProcess->width;
        treeHeight = curProcess->height;

        MPI_Recv(&numCircles, 1, MPI_INT, 0, tag_numCircles, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        circles = (struct Circle*) realloc(circles, numCircles * sizeof(struct Circle)); // FU*K
        MPI_Recv(circles, numCircles * sizeof(struct Circle), MPI_BYTE, 0, tag_circles, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    if (rank == 0) {
        begin = clock();
    }

    while (true) {
        update();

        frames++;
        clock_t end = clock();
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        MPI_Bcast(&time_spent, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        if (time_spent >= 10) {
            if (rank == 0)
                printf("%f FPS\n", frames / time_spent);
            frames = 0;
            begin = end;
            MPI_Finalize();
            exit(0);
        }
    }

    MPI_Finalize();
    return 0;
}

void update() {
    if (rank != 0) {
        rebuildTree();
        MPI_Send(circles, numCircles * sizeof(struct Circle), MPI_BYTE, 0, tag_circles, MPI_COMM_WORLD);
        freeTree(rootCell);
        MPI_Recv(&numCircles, 1, MPI_INT, 0, tag_numCircles, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        circles = (struct Circle*) realloc(circles, numCircles * sizeof(struct Circle)); // FU*K
        MPI_Recv(circles, numCircles * sizeof(struct Circle), MPI_BYTE, 0, tag_circles, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else {
        rebuildTree();
        freeTree(rootCell);

        processes[0].circles = circles;
        for (int j = 0; j < processes[0].numCircles; j++) {
            allCircles[processes[0].circle_ids[j]] = processes[0].circles[j];
        }

        for (int i = 1; i < numProcesses; i++) {
            MPI_Recv(processes[i].circles, processes[i].numCircles * sizeof(struct Circle), MPI_BYTE, i, tag_circles, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int j = 0; j < processes[i].numCircles; j++) {
                allCircles[processes[i].circle_ids[j]] = processes[i].circles[j];
            }
        }

        distributeCircles();
    }
}

void distributeCircles() {
    if (rank == 0) {
        for (int i = 0; i < numProcesses; i++) {
            processes[i].numCircles = 0;
        }

        for (int i = 0; i < numAllCircles; i++) {
            for (int j = 0; j < numProcesses; j++) {
                if (!isCircleOverlappingArea(&allCircles[i], processes[j].posX, processes[j].posY, processes[j].width, processes[j].height))
                    continue;
                processes[j].numCircles++;
            }
        }

        for (int i = 0; i < numProcesses; i++) {
            processes[i].circles = (struct Circle *) realloc(processes[i].circles, processes[i].numCircles * sizeof(struct Circle));
            processes[i].circle_ids = (int *) realloc(processes[i].circle_ids, processes[i].numCircles * sizeof(int));
            processes[i].numCircles = 0;
        }

        for (int i = 0; i < numAllCircles; i++) {
            for (int j = 0; j < numProcesses; j++) {
                if (!isCircleOverlappingArea(&allCircles[i], processes[j].posX, processes[j].posY, processes[j].width, processes[j].height))
                    continue;
                processes[j].circles[processes[j].numCircles] = allCircles[i];
                processes[j].circle_ids[processes[j].numCircles] = i;
                processes[j].numCircles++;
            }
        }

        numCircles = processes[0].numCircles;
        circles = processes[0].circles;
        for (int i = 1; i < numProcesses; i++) {
            MPI_Send(&processes[i].numCircles, 1, MPI_INT, i, tag_numCircles, MPI_COMM_WORLD);
            MPI_Send(processes[i].circles, processes[i].numCircles * sizeof(struct Circle), MPI_BYTE, i, tag_circles, MPI_COMM_WORLD);
        }
    }
}
