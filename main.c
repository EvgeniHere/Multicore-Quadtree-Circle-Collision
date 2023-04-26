#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>

#define SCREEN_WIDTH 20
#define SCREEN_HEIGHT 20

#define numCircles 15 //((SCREEN_HEIGHT - 2) / 2 * (SCREEN_WIDTH - 2) / 2)
#define circleSize 3
#define maxCirclesPerCell 5
#define maxSpeed 1.0
#define force 10.0
#define count 10


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
    int depth;
    bool isLeaf;
    int* circle_ids;
    struct Cell* subCells;
};
void move(int circle_id);
void checkCollisions(int circle_id, struct Cell* cell);
void deleteTree(struct Cell* cell);
bool isCircleInCellArea(int circle_id, struct Cell cell);
void split(struct Cell* cell);
void addCircleToCell(int circle_id, struct Cell* cell);
int random_int(int min, int max);
double random_double(double min, double max);
void save_Iteration(FILE* file);

int main() {

    FILE* file = fopen("../data.txt","w");

    if(file == NULL)
    {
        printf("Error!");
        exit(1);
    }

    //circles =  (struct Circle*)malloc( numCircles * sizeof(struct Circle));


    srand(time(0));

    struct Cell rootCell;
    rootCell.cellWidth = (double) SCREEN_WIDTH;
    rootCell.cellHeight = (double) SCREEN_HEIGHT;
    rootCell.depth = 0;
    rootCell.isLeaf = true;
    rootCell.numCirclesInCell = 0;
    rootCell.circle_ids = (int*)malloc(maxCirclesPerCell * sizeof(int));

    printf("Flakesize: %d\n Num_Flakes: %d\n", circleSize, numCircles);
    fprintf(file, "%d %d %d %d\n", SCREEN_WIDTH, SCREEN_HEIGHT, numCircles, circleSize);

    /*
    int i = 0;
    for (int x = 2; x <= SCREEN_WIDTH-2; x+=2) {
        for (int y = 2; y <= SCREEN_HEIGHT-2; y+=2) {
            circles[i].posX = x;
            circles[i].posY = y;
            circles[i].velX = random_double(-1.0, 1.0);
            circles[i].velY = random_double(-1.0, 1.0);
            addCircleToCell(i, &rootCell);
            i++;
        }
    }
    */
    for (int i = 0; i < numCircles; i++) {
        circles[i].posX = random_int(0, SCREEN_WIDTH);
        circles[i].posY = random_int(0, SCREEN_HEIGHT);
        circles[i].velX = random_double(-1.0, 1.0);
        circles[i].velY = random_double(-1.0, 1.0);
        addCircleToCell(i, &rootCell);
    }


    clock_t begin = clock();
    for (int counter = 0; counter < count; counter++) {
        for (int i = 0; i < numCircles; i++) {
            addCircleToCell(i, &rootCell);
            move(i);
        }
        deleteTree(&rootCell);
        rootCell.circle_ids = (int*)malloc(maxCirclesPerCell * sizeof(int));
        if(counter%10==0)
            save_Iteration(file);

    }
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Time per Iteration: %f\n", time_spent/count);

    fclose(file);


}



void move(int circle_id) {
    struct Circle circle = circles[circle_id];
    if (circle.velX > maxSpeed)
        circle.velX = maxSpeed;
    if (circle.velY > maxSpeed)
        circle.velY = maxSpeed;
    if (circle.velX < -maxSpeed)
        circle.velX = -maxSpeed;
    if (circle.velY < -maxSpeed)
        circle.velY = -maxSpeed;

    if (circle.posX + circle.velX + circleSize > SCREEN_WIDTH) {
        circle.posX = SCREEN_WIDTH - circleSize;
        circle.velX *= -1;
    } else if (circle.posX + circle.velX < 0) {
        circle.posX = 0;
        circle.velX *= -1;
    }

    if (circle.posY + circle.velY + circleSize > SCREEN_HEIGHT) {
        circle.posY = SCREEN_HEIGHT - circleSize;
        circle.velY *= -1;
    } else if (circle.posY + circle.velY < 0) {
        circle.posY = 0;
        circle.velY *= -1;
    }

    circle.posX += circle.velX;
    circle.posY += circle.velY;
}

void checkCollisions(int circle_id, struct Cell* cell) {
    struct Circle* circle1 = &circles[circle_id];
    double midX1 = circle1->posX;
    double midY1 = circle1->posY;

    for (int i = 0; i < cell->numCirclesInCell; i++) {
        /*struct Circle* circle2 = &circles[cell->circle_ids[i]];
        double midX2 = circle2->posX;
        double midY2 = circle2->posX;

        double a = midX2 - midX1;
        double b = midY2 - midY1;
        double c = sqrt(a*a + b*b);

        if (c > circleSize)
            continue;

        double angle = atan2(b, a);
        double targetX = midX1 + cos(angle) * circleSize;
        double targetY = midY1 + sin(angle) * circleSize;
        double ax = (targetX - midX2);
        double ay = (targetY - midY2);

        circles[circle_id].velX -= ax * force;
        circles[circle_id].velY -= ay * force;
        circles[cell->circle_ids[i]].velX += ax * force;
        circles[cell->circle_ids[i]].velY += ay * force;*/
        int j = cell->circle_ids[i];
        double d = sqrt((circles[circle_id].posX - circles[j].posX)*(circles[circle_id].posX - circles[j].posX)  + (circles[circle_id].posY - circles[j].posY)*(circles[circle_id].posY - circles[j].posY));
        if(d > circleSize)
            continue;

        double nx = (circles[j].posX - circles[circle_id].posX) / d;
        double ny = (circles[j].posY - circles[circle_id].posY) / d;
        double p = 2 * (circles[circle_id].velX * nx + circles[circle_id].velY * ny - circles[j].velX * nx -
                circles[j].velY * ny);// / (bowl[id].mass + bowl[j].mass);
        circles[circle_id].velX += -p * nx;
        circles[circle_id].velY += -p * ny;
        circles[j].velX += p * nx;
        circles[j].velY += p * ny;
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
    struct Circle circle = circles[circle_id];

    return circle.posX + circleSize > cell.cellWidth && circle.posX < cell.posX + cell.cellWidth && circle.posY + circleSize > cell.posY && circle.posY < cell.posY + cell.cellHeight;
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
        cell->subCells[i].depth = cell->depth + 1;
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

void addCircleToCell(int circle_id, struct Cell* cell) {
    if (!cell->isLeaf) {
        for (int i = 0; i < 4; i++)
            if (isCircleInCellArea(circle_id, cell->subCells[i]))
                addCircleToCell(circle_id, &cell->subCells[i]);
        return;
    }

    if (cell->numCirclesInCell < maxCirclesPerCell) {
        checkCollisions(circle_id, cell);
        cell->circle_ids[cell->numCirclesInCell] = circle_id;
        cell->numCirclesInCell++;
        return;
    }

    split(cell);

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < maxCirclesPerCell; j++) {
            if (isCircleInCellArea(cell->circle_ids[j], cell->subCells[i]))
                addCircleToCell(cell->circle_ids[j], &cell->subCells[i]);
        }

        //if (isCircleInCellArea(circle_id, cell->subCells[i]))
        //    addCircleToCell(circle_id, &cell->subCells[i]);
    }

    cell->numCirclesInCell = 0;
    free(cell->circle_ids);
}

int random_int(int min, int max) {
    return rand() % (max - min + 1) + min;
}

double random_double(double min, double max) {
    return min + (rand() / (RAND_MAX / (max - min)));
}

void save_Iteration(FILE* file) {
    for(int i=0; i<numCircles; i++) {
        fprintf(file,"%f %f ",circles[i].posX, circles[i].posY);
    }
    fprintf(file,"%s", " \n");
}

