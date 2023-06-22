#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "circle.c"

int numCircles;
struct Circle* circles;
void rebuildTree();
bool isCircleOverlappingArea(struct Circle* circle, double cellPosX, double cellPosY, double cellWidth, double cellHeight);
double random_double(double min, double max);

int treeWidth;
int treeHeight;
int treePosX;
int treePosY;

void checkCollisions() {
    for (int i = 0; i < numCircles; i++) {
        struct Circle* circle1 = &circles[i];
        for (int j = i + 1; j < numCircles; j++) {
            struct Circle* circle2 = &circles[j];

            if (fabs(circle1->posX - circle2->posX) > circleSize ||
                fabs(circle1->posY - circle2->posY) > circleSize)
                continue;

            double dx = circle2->posX - circle1->posX;
            double dy = circle2->posY - circle1->posY;
            double distSquared = dx * dx + dy * dy;
            double sum_r = circleSize;

            if (distSquared < sum_r * sum_r) {
                double dist = sqrt(distSquared);
                double overlap = (sum_r - dist) / 2.0;
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

void rebuildTree() {
    checkCollisions();
}

bool isCircleOverlappingArea(struct Circle* circle, double cellPosX, double cellPosY, double cellWidth, double cellHeight) {
    return circle->posX + circleSize / 2.0 >= cellPosX && circle->posX - circleSize / 2.0 <= cellPosX + cellWidth && circle->posY + circleSize / 2.0 >= cellPosY && circle->posY - circleSize / 2.0 <= cellPosY + cellHeight;
}

double random_double(double min, double max) {
    double range = max - min;
    double scaled = (double)rand() / RAND_MAX;  // random value between 0 and 1
    return min + (scaled * range);
}
