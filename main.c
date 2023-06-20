#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <mpi/mpi.h>
#include <GL/glut.h>
#include <GL/gl.h>


#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 1000
#define numCircles 1000
#define circleSize 10
#define maxSpawnSpeed (circleSize / 4.0)
#define maxSpeed (circleSize / 4.0)

#define friction 1.0


struct Circle {
    double posX;
    double posY;
    double velX;
    double velY;
};

struct Circle* circles;
int world_size;
int world_rank;

void move(int circle_id);

int random_int(int min, int max);
double random_double(double min, double max);
void checkCollisions(int id_1);
void display();
void update(int counter);
void drawCircle(GLdouble centerX, GLdouble centerY, GLdouble radius);
void checkPosition(struct Circle* circle);

void mouseClick(int button, int state, int x, int y) {
    MPI_Finalize();
    exit(0);
}


int main(int argc, char** argv) {
    MPI_Init(NULL, NULL);

    // Get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    //circles =  (struct Circle*)malloc( numCircles * sizeof(struct Circle));
    double intpart;
    if(world_rank == 0 && modf(numCircles/(double)world_size, &intpart) != 0) {
        printf("Number of circles must be dividable by num of processes!\n");
        MPI_Abort(MPI_COMM_WORLD, 3);
        exit(1);
    }

    srand(time(NULL));
    circles = malloc(numCircles * sizeof(struct Circle));

    if(world_rank==0) {
        printf("Circle Size: %d\nNum. Circles: %d\n", (int)circleSize, numCircles);
        for (int i = 0; i < numCircles; i++) {
            circles[i].posX = random_double(circleSize/2.0, SCREEN_WIDTH-circleSize/2.0);
            circles[i].posY = random_double(circleSize/2.0, SCREEN_HEIGHT-circleSize/2.0);
            circles[i].velX = random_double(-maxSpawnSpeed, maxSpawnSpeed);
            circles[i].velY = random_double(-maxSpawnSpeed, maxSpawnSpeed);
        }
    }
    MPI_Bcast(circles, numCircles*4, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    double h = numCircles / (double)world_size;
    if(world_rank==0) {
        glutInit(&argc, argv);
        glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
        glutInitWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
        glutCreateWindow("Bouncing Circles");
        glutDisplayFunc(display);
        glutTimerFunc(16, update, 0);
        glutMouseFunc(mouseClick);
        glutMainLoop();
    }
    else {
        while(true) {
            for (int i = floor(world_rank * h); i < ceil((world_rank + 1) * h); i++) {
                checkCollisions(i);
                move(i);
            }
            MPI_Allgather(&circles[(int)(world_rank * h)], (int)h * 4, MPI_DOUBLE, circles, (int)h * 4, MPI_DOUBLE, MPI_COMM_WORLD);
        }
    }
    MPI_Finalize();
    exit(0);
}
void drawCircle(GLdouble centerX, GLdouble centerY, GLdouble radius) {
    GLdouble angleIncrement = 2.0 * M_PI / 32;
    glBegin(GL_POLYGON);
    for (int i = 0; i < 32; i++) {
        GLdouble angle = i * angleIncrement;
        GLdouble x = centerX + radius * cos(angle);
        GLdouble y = centerY + radius * sin(angle);
        glVertex2d(x, y);
    }
    glEnd();
}


void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, SCREEN_WIDTH, 0, SCREEN_HEIGHT); // Set up an orthographic projection
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Draw three circles at different positions
    for (int i = 0; i < numCircles; i++) {
        GLdouble centerX = circles[i].posX;
        GLdouble centerY = circles[i].posY;
        GLdouble radius = circleSize/2.0;
        drawCircle(centerX, centerY, radius);
    }

    glutSwapBuffers();
}

void update(int counter) {
    int h = numCircles / world_size;
    for (int i = world_rank * h; i < (world_rank + 1) * h; i++) {
        checkCollisions(i);
        move(i);
    }
    MPI_Allgather(&circles[world_rank * h], h * 4, MPI_DOUBLE, circles, h * 4, MPI_DOUBLE, MPI_COMM_WORLD);
    glutPostRedisplay();
    glutTimerFunc(16, update, counter + 1);

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

void checkCollisions(int id_1) {

    for (int id_2 = 0; id_2 < numCircles; id_2++) {
        if (id_1 == id_2)
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

                double dvx = circles[id_2].velX - circles[id_1].velX;
                double dvy = circles[id_2].velY - circles[id_1].velY;
                double dot = dvx * dx + dvy * dy;

                circles[id_1].velX += dot * dx;
                circles[id_1].velY += dot * dy;

                circles[id_1].velX *= friction;
                circles[id_1].velY *= friction;
            }
        }
        checkPosition(&circles[id_1]);
    }
}


int random_int(int min, int max) {
    return rand() % (max - min + 1) + min;
}

double random_double(double min, double max) {
    return (max - min) * ( (double)rand() / (double)RAND_MAX ) + min;
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

