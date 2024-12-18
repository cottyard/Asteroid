#include <string>
#define GLUT_DISABLE_ATEXIT_HACK
#include "GL/freeglut.h"
#include "Common.h"

int windowNumber;
int lastUpdateTime = 0;

World world;
World initWorld();

void onKeyPressed(unsigned char key, int px, int py) {
    switch (key) {
    case 27:
        exit(0);
        break;
    case ' ':
        if (world.alive) world.shooting = true;
        else world = initWorld();
        break;        
    }
}

void onKeyUp(unsigned char key, int px, int py) {
    switch (key) {
    case ' ':
        world.shooting = false;
        break;
    }
}

void onSpecialKeyPressed(int key, int x, int y){
    int time = glutGet(GLUT_ELAPSED_TIME);
    switch (key){
    case GLUT_KEY_UP:
        world.thrusting = true;
        break;
    case GLUT_KEY_LEFT:
        world.turning_left = true;
        break;
    case GLUT_KEY_RIGHT:
        world.turning_right = true;
        break;
    }
}

void onSpecialKeyUp(int key, int x, int y){
    switch (key){
    case GLUT_KEY_UP:
        world.thrusting = false;
        break;
    case GLUT_KEY_LEFT:
        world.turning_left = false;
        break;
    case GLUT_KEY_RIGHT:
        world.turning_right = false;
        break;
    }
}

void displayScore(){
    glColor3f(0.0, 1.0, 1.0);
    glRasterPos2i(1, ORTHO_MAX - 3);
    std::string s = "Score: ";
    s += std::to_string(world.score);
    int i = 0;
    while (i < s.size()) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, s[i]);
        i++;
    }
}

void stepWorld(World& world, int delta);
void onUpdate() {
    int time = glutGet(GLUT_ELAPSED_TIME);
    stepWorld(world, time - lastUpdateTime);
    lastUpdateTime = time;
    glutPostWindowRedisplay(windowNumber);
}

void drawWorld(World world);
void onDisplay(){
    glClear(GL_COLOR_BUFFER_BIT);
    drawWorld(world);
    displayScore();
    glutSwapBuffers();
}

void onVisibilityChange(int vis){
    if (vis == GLUT_VISIBLE) glutIdleFunc(&onUpdate);
    else glutIdleFunc(nullptr);
}

int main(int argc, char** argv)
{
	world = initWorld();
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 800);
    windowNumber = glutCreateWindow("Asteroids Example for C++ Beginners");
    glutIgnoreKeyRepeat(1);
    glutKeyboardFunc(&onKeyPressed);
    glutKeyboardUpFunc(&onKeyUp);
    glutSpecialFunc(&onSpecialKeyPressed);
    glutSpecialUpFunc(&onSpecialKeyUp);
    glutDisplayFunc(&onDisplay);
    glutVisibilityFunc(&onVisibilityChange);
    glOrtho(0, ORTHO_MAX, 0, ORTHO_MAX, -1.0, 1.0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glutMainLoop();
    return 0;
}

