#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <mpi/mpi.h>
#include <GL/glut.h>
#include <GL/gl.h>


#define SCREEN_WIDTH 30
#define SCREEN_HEIGHT 20
#define spaceBetweenCircles 2
#define numCircles ((SCREEN_HEIGHT - spaceBetweenCircles) / spaceBetweenCircles * (SCREEN_WIDTH - spaceBetweenCircles) / spaceBetweenCircles)
#define circleSize 0.5f
#define maxCirclesPerCell 5
#define maxSpeed 1.0f
#define force 10.0
#define count 100
#define saveIntervall 1
#define dt 0.1f


struct Circle {
    double posX;
    double posY;
    double velX;
    double velY;
};

struct Circle* circles;
FILE* file;
int world_size;
int world_rank;

void move(int circle_id);

int random_int(int min, int max);
double random_double(double min, double max);
void save_Iteration(FILE* file);
void checkCollisions(int circle_id);
void display();
void update(int counter);
void drawCircle(GLdouble centerX, GLdouble centerY, GLdouble radius);

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

    if(world_rank==0) {
        file = fopen("../data.txt", "w");
        if(file == NULL) {
            printf("Error!");
            MPI_Abort(MPI_COMM_WORLD, 1);
            exit(1);
        }
    }



    //circles =  (struct Circle*)malloc( numCircles * sizeof(struct Circle));

    srand(time(NULL));
    circles = malloc(numCircles * sizeof(struct Circle));

    if(world_rank==0) {
        printf("Circle Size: %d\nNum. Circles: %d\n", (int)circleSize, numCircles);
        fprintf(file, "%d %d %d %d %d\n", SCREEN_WIDTH, SCREEN_HEIGHT, numCircles, (int)circleSize, count/saveIntervall);
        int i = 0;
        for (int x = spaceBetweenCircles; x <= SCREEN_WIDTH - spaceBetweenCircles; x += spaceBetweenCircles) {
            for (int y = spaceBetweenCircles; y <= SCREEN_HEIGHT - spaceBetweenCircles; y += spaceBetweenCircles) {
                circles[i].posX = x;
                circles[i].posY = y;
                circles[i].velX = random_double(-1.0, 1.0);
                circles[i].velY = random_double(-1.0, 1.0);
                i++;
            }
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

    //TODO Exit
    if(world_rank==0) {
        save_Iteration(file);
    }
    if(world_rank==0) {
        fclose(file);
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
        GLdouble radius = circleSize;
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
    if (world_rank == 0 && counter % saveIntervall == 0) {
        save_Iteration(file);
        fflush(file);
    }
    glutPostRedisplay();
    glutTimerFunc(16, update, counter + 1);

    //if (counter % saveIntervall == 0)
    //    save_Iteration(file);

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
    for (int j = 0; j < numCircles; j++) {
        if(j == circle_id)
            continue;

        double dist = sqrt(
                pow(circles[j].posX - circles[circle_id].posX, 2) + pow(circles[j].posY - circles[circle_id].posY, 2));
        double sum_r = circleSize * 2;

        if (dist < sum_r) {
            double d = dist - sum_r;
            double dx = (circles[j].posX - circles[circle_id].posX) / dist;
            double dy = (circles[j].posY - circles[circle_id].posY) / dist;

            circles[circle_id].posX += d * dx;
            circles[circle_id].posY += d * dy;
            circles[j].posX -= d * dx;
            circles[j].posY -= d * dy;

            double v1x = circles[circle_id].velX;
            double v1y = circles[circle_id].velY;
            double v2x = circles[j].velX;
            double v2y = circles[j].velY;

            double v1x_new = v1x - 2 * circleSize / sum_r * (v1x * dx + v1y * dy - v2x * dx - v2y * dy) * dx;
            double v1y_new = v1y - 2 * circleSize / sum_r * (v1x * dx + v1y * dy - v2x * dx - v2y * dy) * dy;
            double v2x_new = v2x - 2 * circleSize / sum_r * (v2x * dx + v2y * dy - v1x * dx - v1y * dy) * dx;
            double v2y_new = v2y - 2 * circleSize / sum_r * (v2x * dx + v2y * dy - v1x * dx - v1y * dy) * dy;

            circles[circle_id].velX = v1x_new;
            circles[circle_id].velY = v1y_new;
            circles[j].velX = v2x_new;
            circles[j].velY = v2y_new;


        }
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
