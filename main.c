#include<stdio.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <time.h>

#define SCREEN_WIDTH 1000.0
#define SCREEN_HEIGHT 1000.0
#define numCircles 6000
#define circleSize 5
#define maxCirclesPerCell 3
#define maxSpawnSpeed (circleSize / 4.0)
#define maxSpeed (circleSize / 4.0)
#define gravity 0.01


// Klasse für Kugel (=Flake)
struct Circle {
    double posX;
    double posY;
    double velX;
    double velY;
};
struct Circle circles[numCircles];
void checkCollisions(int circle_id);
double random_double(double min, double max);
void move(int cirle_id);

int frames = 0;
double friction = 0.9;
bool gravityState = false;

int main() {

    srand(90);

    for (int i = 0; i < numCircles; i++) {
        circles[i].posX = random_double(circleSize/2.0, SCREEN_WIDTH-circleSize/2.0);
        circles[i].posY = random_double(circleSize/2.0, SCREEN_HEIGHT-circleSize/2.0);
        circles[i].velX = random_double(-maxSpawnSpeed, maxSpawnSpeed);
        circles[i].velY = random_double(-maxSpawnSpeed, maxSpawnSpeed);
    }
    clock_t begin = clock();
    while(true)
    {
        for(int i=0; i < numCircles; i++) {
            checkCollisions(i);
            move(i);
        }
        frames++;
        clock_t end = clock();
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        if (time_spent >= 10) {
            printf("%f fps\n", frames/time_spent);
            frames = 0;
            begin = end;
            exit(0);
        }



    }

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


void checkCollisions(int  circle_id) {
    int id_1 = circle_id;

    for (int j = 0; j < numCircles; j++) {
        int id_2 = j;
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
                circles[id_2].velX -= dot * dx;
                circles[id_2].velY -= dot * dy;

                circles[id_1].velX *= friction;
                circles[id_1].velY *= friction;
                circles[id_2].velX *= friction;
                circles[id_2].velY *= friction;
            }
        }
    }
    checkPosition(&circles[circle_id]);
}

double random_double(double min, double max) {
    double range = max - min;
    double scaled = (double)rand() / RAND_MAX;  // random value between 0 and 1
    return min + (scaled * range);
}
