#include <string>
#define GLUT_DISABLE_ATEXIT_HACK
#include "GL/freeglut.h"
#include "Common.h"
#include "NeuNet.h"

int windowWorld, windowNeural;
int lastUpdateTime = 0;
bool neuralControlEnabled = false; // Toggle for neural network control

World world;
World initWorld();

void onKeyPressed(unsigned char key, int px, int py) {
    switch (key) {
    case 27:
        exit(0);
        break;
    case ' ':
        if (world.alive) world.control.shooting = true;
        else world = initWorld();
        break;
    case 'n': // Toggle neural network control
    case 'N':
        neuralControlEnabled = !neuralControlEnabled;
        if (neuralControlEnabled) {
            initializeNeuralNetwork();
            printf("Neural network control enabled\n");
        } else {
            printf("Manual control enabled\n");
        }
        break;
    }
}

void onKeyUp(unsigned char key, int px, int py) {
    switch (key) {
    case ' ':
        world.control.shooting = false;
        break;
    }
}

void onSpecialKeyPressed(int key, int x, int y){
    if (!neuralControlEnabled) { // Only allow manual control when neural network is disabled
        int time = glutGet(GLUT_ELAPSED_TIME);
        switch (key){
        case GLUT_KEY_UP:
            world.control.thrusting = true;
            break;
        case GLUT_KEY_LEFT:
            world.control.turningLeft = true;
            break;
        case GLUT_KEY_RIGHT:
            world.control.turningRight = true;
            break;
        }
    }
}

void onSpecialKeyUp(int key, int x, int y){
    if (!neuralControlEnabled) { // Only allow manual control when neural network is disabled
        switch (key){
        case GLUT_KEY_UP:
            world.control.thrusting = false;
            break;
        case GLUT_KEY_LEFT:
            world.control.turningLeft = false;
            break;
        case GLUT_KEY_RIGHT:
            world.control.turningRight = false;
            break;
        }
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
    
    // Display control mode
    glRasterPos2i(1, ORTHO_MAX - 6);
    std::string mode = neuralControlEnabled ? "Neural Control (N to toggle)" : "Manual Control (N to toggle)";
    i = 0;
    while (i < mode.size()) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, mode[i]);
        i++;
    }
}

void stepWorld(World& world, int delta);
void onUpdate() {
    int time = glutGet(GLUT_ELAPSED_TIME);
    
    // Update neural network control if enabled
    if (neuralControlEnabled) {
        updateNeuralControl(world);
    }
    
    stepWorld(world, time - lastUpdateTime);
    lastUpdateTime = time;
    glutPostWindowRedisplay(windowWorld);
    glutPostWindowRedisplay(windowNeural); // Also update neural network display
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

    if (g_neuralNetwork && neuralControlEnabled) {
        // Get current sensor inputs and outputs
        std::vector<float> inputs = getSensorInputs(world);
        std::vector<float> outputs = g_neuralNetwork->feedForward(inputs);
        
        // Show the actual network being used
        std::vector<size_t> config = {13, 8, 4};  // Input: 13, Hidden: 8, Output: 4
        drawNeuralNetwork(config, g_neuralNetwork->weights, inputs, outputs, 800.0f, 800.0f, true);
    } else {
        // Show example network when neural control is disabled
        std::vector<size_t> config = {13, 8, 4};
        std::vector<Eigen::MatrixXf> weights = {
            Eigen::MatrixXf::Random(8, 14),  // Random weights (input + bias -> hidden)
            Eigen::MatrixXf::Random(4, 9)   // Random weights (hidden + bias -> output)
        };
        std::vector<float> inputs(13, 0.0f);
        std::vector<float> outputs(4, 0.0f);
        drawNeuralNetwork(config, weights, inputs, outputs, 800.0f, 800.0f, true);
    }

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

