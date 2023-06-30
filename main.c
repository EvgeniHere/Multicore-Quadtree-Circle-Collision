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
int tag_numCells = 4;
int tag_cells = 5;
int tag_circle_inside = 6;

int frames = 0;
clock_t begin;

int rank, size;

int numProcesses;

int main(int argc, char** argv);
void update();
void distributeCircles();

struct Process {
    int posX;
    int posY;
    int width;
    int height;
    int numCells;
    bool* circle_inside;
    struct Circle* circles;
    struct Rectangle* rects;
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
    numCircles = 100000;
    circleSize = 1.0;
    maxSpeed = 1.0;
    maxCirclesPerCell = 100;
    minCellSize = 2 * circleSize;
    circle_max_X = SCREEN_WIDTH;
    circle_max_y = SCREEN_HEIGHT;

    circles = (struct Circle *) malloc(sizeof(struct Circle) * numCircles);
    circle_inside = (bool *) malloc(sizeof(bool) * numCircles);
    curProcess = (struct Process*) malloc(sizeof(struct Process));

    if (rank == 0) {
        srand(90);
        for (int i = 0; i < numCircles; i++) {
            circles[i].posX = random_double(circleSize / 2.0, SCREEN_WIDTH - circleSize / 2.0);
            circles[i].posY = random_double(circleSize / 2.0, SCREEN_HEIGHT - circleSize / 2.0);
            circles[i].velX = random_double(-maxSpeed, maxSpeed);
            circles[i].velY = random_double(-maxSpeed, maxSpeed);
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
            processes[i].rects = (struct Rectangle *) malloc(sizeof(struct Rectangle));
            processes[i].circle_inside = (bool *) malloc(numCircles * sizeof(bool));
            processes[i].circles = (struct Circle *) malloc(numCircles * sizeof(struct Circle));
            processes[i].numCells = 0;
            if (i > 0)
                MPI_Send(&processes[i], sizeof(struct Process), MPI_BYTE, i, tag_process, MPI_COMM_WORLD);
        }
        for (int i = 0; i < numProcesses; i++) {
            for (int j = 0; j < numCircles; j++) {
                processes[i].circle_inside[j] = isCircleOverlappingArea(&circles[j], processes[i].posX, processes[i].posY, processes[i].width, processes[i].height);
            }
        }
        circle_inside = processes[0].circle_inside;
        curProcess = &processes[0];
    } else {
        MPI_Recv(curProcess, sizeof(struct Process), MPI_BYTE, 0, tag_process, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    MPI_Bcast(circles, numCircles * sizeof(struct Circle), MPI_BYTE, 0, MPI_COMM_WORLD);
    setupQuadtree(curProcess->posX, curProcess->posY, curProcess->width, curProcess->height);

    if (rank == 0)
        begin = clock();

    while (true) {
        update();
    }

    MPI_Finalize();
    return 0;
}

void update() {
    MPI_Bcast(circles, numCircles * sizeof(struct Circle), MPI_BYTE, 0, MPI_COMM_WORLD);
    updateTree();

    if (rank != 0) {
        //MPI_Send(&numCells, 1, MPI_INT, 0, tag_numCells, MPI_COMM_WORLD);
        //MPI_Send(leaf_rects, numCells * sizeof(struct Rectangle), MPI_BYTE, 0, tag_cells, MPI_COMM_WORLD);
        MPI_Send(circles, numCircles * sizeof(struct Circle), MPI_BYTE, 0, tag_circles, MPI_COMM_WORLD);
    } else {
        //processes[0].numCells = numCells;
        //processes[0].rects = (struct Rectangle*) realloc(processes[0].rects, processes[0].numCells * sizeof(struct Rectangle));
        processes[0].circles = circles;
        for (int i = 0; i < numProcesses; i++) {
            //MPI_Recv(&processes[i].numCells, 1, MPI_INT, i, tag_numCells, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            //processes[i].rects = (struct Rectangle*) realloc(processes[i].rects, processes[i].numCells * sizeof(struct Rectangle));
            //MPI_Recv(processes[i].rects, processes[i].numCells * sizeof(struct Rectangle), MPI_BYTE, i, tag_cells, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (i > 0)
                MPI_Recv(processes[i].circles, numCircles * sizeof(struct Circle), MPI_BYTE, i, tag_circles, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int j = 0; j < numCircles; j++) {
                if (processes[i].circle_inside[j])
                    circles[j] = processes[i].circles[j];
                processes[i].circle_inside[j] = isCircleOverlappingArea(&circles[j], processes[i].posX, processes[i].posY, processes[i].width, processes[i].height);
            }
        }
        circle_inside = processes[0].circle_inside;
    }

    if (rank == 0) {
        frames++;
        clock_t end = clock();
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        if (time_spent >= 10) {
            printf("%d frames for 10 seconds\n", frames);
            printf("%f FPS\n", frames / 10.0);
            frames = 0;
            begin = end;
            //MPI_Finalize();
            //exit(0);
        }
    }
}
