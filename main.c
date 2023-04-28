#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <mpi/mpi.h>

#define SCREEN_WIDTH 20
#define SCREEN_HEIGHT 20
#define numCircles ((SCREEN_HEIGHT - 2) / 2 * (SCREEN_WIDTH - 2) / 2)
#define circleSize 1.0
#define maxCirclesPerCell 5
#define maxSpeed 1.0
#define force 10.0
#define count 100
#define saveIntervall 1
#define dt 0.1


struct Circle {
    double posX;
    double posY;
    double velX;
    double velY;
};

struct Circle* circles;



void move(int circle_id);

int random_int(int min, int max);
double random_double(double min, double max);
void save_Iteration(FILE* file);
void checkCollisions(int circle_id);

int main() {
    MPI_Init(NULL, NULL);

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    FILE *file;
    if(world_rank==0) {
         file = fopen("../data.txt", "w");
        if(file == NULL) {
            printf("Error!");
            MPI_Abort(MPI_COMM_WORLD, 1);
            exit(1);
        }
    }



    //circles =  (struct Circle*)malloc( numCircles * sizeof(struct Circle));

    srand(20);
    circles = malloc(numCircles * sizeof(struct Circle));

    if(world_rank==0) {
        printf("Circle Size: %d\nNum. Circles: %d\n", (int)circleSize, numCircles);
        fprintf(file, "%d %d %d %d %d\n", SCREEN_WIDTH, SCREEN_HEIGHT, numCircles, (int)circleSize, count/saveIntervall);
        int i = 0;
        for (int x = 2; x <= SCREEN_WIDTH - 2; x += 2) {
            for (int y = 2; y <= SCREEN_HEIGHT - 2; y += 2) {
                circles[i].posX = x;
                circles[i].posY = y;
                circles[i].velX = random_double(-1.0, 1.0);
                circles[i].velY = random_double(-1.0, 1.0);
                i++;
            }
        }
    }
    MPI_Bcast(circles, numCircles*4, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    /*

    for (int i = 0; i < numCircles; i++) {
        circles[i].posX = random_double(circleSize/2, SCREEN_WIDTH-circleSize/2);
        circles[i].posY = random_double(circleSize/2, SCREEN_HEIGHT-circleSize/2);
        circles[i].velX = random_double(-1.0, 1.0);
        circles[i].velY = random_double(-1.0, 1.0);
    }*/
    if(world_rank==0) {
        save_Iteration(file);
    }
    clock_t begin = clock();
    int h = numCircles/world_size;
    for (int counter = 0; counter < count; counter++) {
        for (int i = world_rank*h; i < (world_rank+1)*h; i++) {
            checkCollisions(i);
            move(i);
        }
        MPI_Allgather(circles, h*4, MPI_DOUBLE, circles, h*4, MPI_DOUBLE, MPI_COMM_WORLD);

        if(world_rank==0 && counter%saveIntervall==0) {
            save_Iteration(file);
            fflush(file);
        }

    }
    if(world_rank==0) {
        clock_t end = clock();
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("Time per Iteration: %f\n", time_spent/count);
        fclose(file);
    }
    MPI_Finalize();
    exit(0);
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

void checkCollisions(int circle_id) {
    struct Circle* circle1 = &circles[circle_id];
    double midX1 = circle1->posX;
    double midY1 = circle1->posY;

    for (int j = 0; j < numCircles; j++) {
        /*
        struct Circle* circle2 = &circles[cell->circle_ids[i]];
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
        circles[cell->circle_ids[i]].velY += ay * force;
        */
        if(j == circle_id)
            continue;
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
