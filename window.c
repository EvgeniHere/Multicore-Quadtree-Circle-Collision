#include <GL/glut.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 1000

void (*updateFunc)(int);

void initOpenGL(int* argc, char** argv, void (*callbackDisplayFunc)(void), void (*callbackTimerFunc)(int), void (*callbackMouseFunc)(int, int, int, int)) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutInitWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
    glutCreateWindow("Bouncing Circles");
    glutDisplayFunc(callbackDisplayFunc);
    updateFunc = callbackTimerFunc;
    glutTimerFunc(16, callbackTimerFunc, 0);
    glutMouseFunc(callbackMouseFunc);
    glutMainLoop();
}

void updateWindow(double fps, int counter) {
    glutPostRedisplay();
    glutTimerFunc(fps, updateFunc, counter + 1);
}
