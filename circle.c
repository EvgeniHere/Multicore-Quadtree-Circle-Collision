#include <stdbool.h>

#define SCREEN_WIDTH 2300.0
#define SCREEN_HEIGHT 1300.0

struct Circle {
    int id;
    double posX;
    double posY;
    double velX;
    double velY;
};

double circleSize = 0;
double maxSpeed = 0;

double friction = 0.999;
double gravity = 0.00001;
bool gravityState = true;

int circle_max_X = 0;
int circle_max_y = 0;

void move(struct Circle* circle);
struct Circle* circleCopy(struct Circle* circle);

void move(struct Circle* circle) {
    if (gravityState) {
        /*if (circle->posX > SCREEN_WIDTH / 2.0) {
            circle->velX -= gravity;
        } else {
            circle->velX += gravity;
        }
        if (circle->posY > SCREEN_HEIGHT / 2.0) {
            circle->velY -= gravity;
        } else {
            circle->velY += gravity;
        }*/
        circle->velX -= gravity*(circle->posX - SCREEN_WIDTH / 2.0);
        circle->velY -= gravity*(circle->posY - SCREEN_HEIGHT / 2.0);
    }

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

    if (circle->posX + circleSize/2.0 + stepDistX > circle_max_X && circle->velX > 0.0) {
        circle->posX = circle_max_X - circleSize/2.0 - stepDistX;
        circle->velX *= -friction;
    } else if (circle->posX - circleSize/2.0 + stepDistX < 0.0 && circle->velX < 0.0) {
        circle->posX = circleSize/2.0 - stepDistX;
        circle->velX *= -friction;
    }

    if (circle->posY + circleSize/2.0 + stepDistY > circle_max_y && circle->velY > 0.0) {
        circle->posY = circle_max_y - circleSize/2.0 - stepDistY;
        circle->velY *= -friction;
    } else if (circle->posY - circleSize/2.0 + stepDistY < 0.0 && circle->velY < 0.0) {
        circle->posY = circleSize/2.0 - stepDistY;
        circle->velY *= -friction;
    }

    circle->posX += circle->velX;
    circle->posY += circle->velY;
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
