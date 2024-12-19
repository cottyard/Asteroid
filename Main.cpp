#include <string>
#define GLUT_DISABLE_ATEXIT_HACK
#include "GL/freeglut.h"
#include "Common.h"

int windowWorld, windowNeural;
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
    glutPostWindowRedisplay(windowWorld);
}

void drawWorld(World world);
void onDisplayWorld(){
    glClear(GL_COLOR_BUFFER_BIT);
    drawWorld(world);
    displayScore();
    glutSwapBuffers();
}

#include <Eigen/Dense>
void drawNeuralNetwork(
    const std::vector<size_t>& config,
    const std::vector<Eigen::MatrixXf>& weights,
    const std::vector<float>& inputs,
    const std::vector<float>& outputs,
    float width, float height, bool bias
);

void onDisplayNeural() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Example network parameters
    std::vector<size_t> config = {3, 4, 2};  // Input layer: 3, Hidden: 4, Output: 2
    std::vector<Eigen::MatrixXf> weights = {
        Eigen::MatrixXf::Random(4, 4),  // Random weights between input and hidden
        Eigen::MatrixXf::Random(2, 5)   // Random weights between hidden and output
    };
    std::vector<float> inputs = {0.5f, 0.2f, 0.8f};
    std::vector<float> outputs = {0.7f, 0.3f};

    drawNeuralNetwork(config, weights, inputs, outputs, 800.0f, 800.0f, true);

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
    
    glutInitWindowPosition(800, 100);
    windowNeural = glutCreateWindow("Neural");
    glutDisplayFunc(onDisplayNeural);
    glOrtho(-800, 800, -800, 800, -1.0, 1.0);
    
    glutInitWindowPosition(0, 100);
    windowWorld = glutCreateWindow("World");
    glutDisplayFunc(onDisplayWorld);
    glOrtho(0, ORTHO_MAX, 0, ORTHO_MAX, -1.0, 1.0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glutIgnoreKeyRepeat(1);
    glutKeyboardFunc(onKeyPressed);
    glutKeyboardUpFunc(onKeyUp);
    glutSpecialFunc(onSpecialKeyPressed);
    glutSpecialUpFunc(onSpecialKeyUp);
    glutVisibilityFunc(onVisibilityChange);
    
    glutMainLoop();
    return 0;
}

