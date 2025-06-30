#include <string>
#include <fstream>
#define GLUT_DISABLE_ATEXIT_HACK
#include "GL/freeglut.h"
#include "Common.h"
#include "NeuNet.h"
#include "GeneticAlgorithm.h"

int windowWorld, windowNeural;
int lastUpdateTime = 0;
bool neuralControlEnabled = false; // Toggle for neural network control
bool geneticTrainingMode = false; // Toggle for genetic training mode

World world;
World initWorld();

void onKeyPressed(unsigned char key, int px, int py) {
    switch (key) {
    case 27:
        exit(0);
        break;
    case ' ':
        if (!geneticTrainingMode) {
            if (world.alive) world.control.shooting = true;
            else world = initWorld();
        }
        break;
    case 'n': // Toggle neural network control
    case 'N':
        if (!geneticTrainingMode) {
            neuralControlEnabled = !neuralControlEnabled;
            if (neuralControlEnabled) {
                initializeNeuralNetwork();
                printf("Neural network control enabled\n");
            } else {
                printf("Manual control enabled\n");
            }
        }
        break;
    case 'g': // Toggle genetic training
    case 'G':
        geneticTrainingMode = !geneticTrainingMode;
        if (geneticTrainingMode) {
            initializeGeneticTraining();
            neuralControlEnabled = false; // Disable single neural control
            printf("Genetic training mode enabled\n");
        } else {
            // Stop genetic training
            if (isGeneticTrainingActive()) {
                toggleGeneticTraining();
            }
            printf("Genetic training mode disabled\n");
            world = initWorld(); // Reset world for manual play
        }
        break;
    case 'r': // Reset training/world
    case 'R':
        if (geneticTrainingMode && g_population) {
            printf("Restarting genetic training...\n");
            initializeGeneticTraining();
        } else {
            world = initWorld();
        }
        break;
    case 'l': // Load neural network from file
    case 'L':
        if (!geneticTrainingMode) {
            // For now, load the most recent best neural network
            // You can modify this to show a file selection dialog or load a specific file
            printf("Enter generation number to load (0-99), or press 'q' to cancel: ");
            // For demonstration, let's try to load the latest generation
            // In a real implementation, you might want to implement a simple file selection
            
            // Try to find the latest generation file
            for (int i = 1; i <= 10; i++) {
                std::string filename = "brain/elite_" + std::to_string(i) + ".json";
                std::ifstream file(filename);
                if (file.good()) {
                    file.close();
                    if (loadNeuralNetworkFromFile(filename)) {
                        neuralControlEnabled = true;
                        world = initWorld(); // Reset world for testing
                        printf("Loaded neural network from generation %d. Neural control enabled for testing.\n", i);
                        printf("Press 'N' to disable neural control.\n");
                    }
                    break;
                }
            }
        }
        break;
    }
}

void onKeyUp(unsigned char key, int px, int py) {
    switch (key) {
    case ' ':
        if (!geneticTrainingMode) {
            world.control.shooting = false;
        }
        break;
    }
}

void onSpecialKeyPressed(int key, int x, int y){
    if (!neuralControlEnabled && !geneticTrainingMode) { // Only allow manual control when neural network and genetic training are disabled
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
    } else if (geneticTrainingMode && g_population) {
        // Manual individual switching controls during genetic training
        switch (key) {
        case GLUT_KEY_LEFT:
            // Switch to previous individual
            {
                size_t current = g_population->getTrackedIndex();
                size_t prev = (current == 0) ? g_population->getPopulationSize() - 1 : current - 1;
                g_population->setTrackedIndividual(prev);
                printf("Switched to individual #%zu\n", prev);
            }
            break;
        case GLUT_KEY_RIGHT:
            // Switch to next individual
            {
                size_t current = g_population->getTrackedIndex();
                size_t next = (current + 1) % g_population->getPopulationSize();
                g_population->setTrackedIndividual(next);
                printf("Switched to individual #%zu\n", next);
            }
            break;
        case GLUT_KEY_UP:
            // Switch to best alive individual
            g_population->trackBestAlive();
            printf("Switched to best alive individual #%zu\n", g_population->getTrackedIndex());
            break;
        }
    }
}

void onSpecialKeyUp(int key, int x, int y){
    if (!neuralControlEnabled && !geneticTrainingMode) { // Only allow manual control when neural network and genetic training are disabled
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
    std::string mode;
    if (geneticTrainingMode) {
        mode = "Genetic Training (G=toggle, R=restart)";
    } else if (neuralControlEnabled) {
        mode = "Neural Control (N=toggle, L=load saved brain)";
    } else {
        mode = "Manual Control (N=Neural, G=Genetic, L=Load brain)";
    }
    i = 0;
    while (i < mode.size()) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, mode[i]);
        i++;
    }
    
    // Display genetic training stats if active
    if (geneticTrainingMode && g_population) {
        glRasterPos2i(1, ORTHO_MAX - 9);
        std::string gen_info = "Generation: " + std::to_string(g_population->getCurrentGeneration());
        i = 0;
        while (i < gen_info.size()) {
            glutBitmapCharacter(GLUT_BITMAP_9_BY_15, gen_info[i]);
            i++;
        }
        
        glRasterPos2i(1, ORTHO_MAX - 12);
        std::string fitness_info = "Best Fitness: " + std::to_string(static_cast<int>(g_population->getBestFitness()));
        i = 0;
        while (i < fitness_info.size()) {
            glutBitmapCharacter(GLUT_BITMAP_9_BY_15, fitness_info[i]);
            i++;
        }
        
        // Display tracked individual details
        Individual* tracked = g_population->getTrackedIndividual();
        if (tracked) {
            glRasterPos2i(1, ORTHO_MAX - 15);
            std::string individual_info = "Watching Individual #" + std::to_string(g_population->getTrackedIndex());
            i = 0;
            while (i < individual_info.size()) {
                glutBitmapCharacter(GLUT_BITMAP_9_BY_15, individual_info[i]);
                i++;
            }
            
            glRasterPos2i(1, ORTHO_MAX - 18);
            std::string fitness_detail = "Fitness: " + std::to_string(static_cast<int>(tracked->fitness)) + 
                                       " | Lifespan: " + std::to_string(tracked->lifespan);
            i = 0;
            while (i < fitness_detail.size()) {
                glutBitmapCharacter(GLUT_BITMAP_9_BY_15, fitness_detail[i]);
                i++;
            }
            
            glRasterPos2i(1, ORTHO_MAX - 21);
            std::string status_info = std::string("Destroyed: ") + std::to_string(tracked->asteroids_destroyed);
            i = 0;
            while (i < status_info.size()) {
                glutBitmapCharacter(GLUT_BITMAP_9_BY_15, status_info[i]);
                i++;
            }
        }
    }
}

void stepWorld(World& world, int delta);
void onUpdate() {
    int time = glutGet(GLUT_ELAPSED_TIME);
    
    if (geneticTrainingMode) {
        // Update genetic training
        updateGeneticTraining();
        
        // Update world with the tracked individual's world
        if (g_population && g_population->getTrackedIndividual()) {
            world = g_population->getTrackedIndividual()->world;
        }
    } else {
        // Update neural network control if enabled
        if (neuralControlEnabled) {
            updateNeuralControl(world);
        }
        
        stepWorld(world, time - lastUpdateTime);
    }
    
    lastUpdateTime = time;
    glutPostWindowRedisplay(windowWorld);
    glutPostWindowRedisplay(windowNeural); // Also update neural network display
}

void drawWorld(World world);

// Function to get the current world state for display
World getCurrentDisplayWorld() {
    if (geneticTrainingMode && g_population && g_population->getTrackedIndividual()) {
        return g_population->getTrackedIndividual()->world;
    }
    return world;
}

void onDisplayWorld(){
    glClear(GL_COLOR_BUFFER_BIT);
    drawWorld(getCurrentDisplayWorld());
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

// Function to get neural network data for display
struct NeuralDisplayData {
    std::vector<size_t> config;
    std::vector<Eigen::MatrixXf> weights;
    std::vector<float> inputs;
    std::vector<float> outputs;
    bool valid;
};

NeuralDisplayData getCurrentNeuralDisplayData() {
    NeuralDisplayData data;
    data.valid = false;
    
    if (geneticTrainingMode && g_population && g_population->getTrackedIndividual()) {
        // Show the neural network of the currently tracked individual
        Individual* tracked = g_population->getTrackedIndividual();
        data.inputs = getSensorInputs(tracked->world);
        data.outputs = tracked->brain->feedForward(data.inputs);
        
        // Convert the biased config back to original config for display
        for (size_t i = 0; i < tracked->brain->config.size(); ++i) {
            if (i < tracked->brain->config.size() - 1) {
                data.config.push_back(tracked->brain->config[i] - 1); // Remove bias
            } else {
                data.config.push_back(tracked->brain->config[i]); // Output layer has no bias
            }
        }
        
        data.weights = tracked->brain->weights;
        data.valid = true;
    } else if (g_neuralNetwork && neuralControlEnabled) {
        // Get current sensor inputs and outputs
        data.inputs = getSensorInputs(world);
        data.outputs = g_neuralNetwork->feedForward(data.inputs);
        
        // Convert the biased config back to original config for display
        for (size_t i = 0; i < g_neuralNetwork->config.size(); ++i) {
            if (i < g_neuralNetwork->config.size() - 1) {
                data.config.push_back(g_neuralNetwork->config[i] - 1); // Remove bias
            } else {
                data.config.push_back(g_neuralNetwork->config[i]); // Output layer has no bias
            }
        }
        
        data.weights = g_neuralNetwork->weights;
        data.valid = true;
    } else {
        // Show example 4-layer network when neural control is disabled
        data.config = {13, 8, 8, 4};
        data.weights = {
            Eigen::MatrixXf::Random(8, 14),  // Random weights (input + bias -> hidden1)
            Eigen::MatrixXf::Random(8, 9),   // Random weights (hidden1 + bias -> hidden2)
            Eigen::MatrixXf::Random(4, 9)    // Random weights (hidden2 + bias -> output)
        };
        data.inputs = std::vector<float>(13, 0.0f);
        data.outputs = std::vector<float>(4, 0.0f);
        data.valid = true;
    }
    
    return data;
}

void onDisplayNeural() {
    glClear(GL_COLOR_BUFFER_BIT);

    NeuralDisplayData data = getCurrentNeuralDisplayData();
    if (data.valid) {
        drawNeuralNetwork(data.config, data.weights, data.inputs, data.outputs, 1000.0f, 1000.0f, true);
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

