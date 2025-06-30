#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <Eigen/Dense>
#include <GL/freeglut.h>
#include "NeuNet.h"
#include "Common.h"

// Use the PI constant from Common.h instead of defining our own

// Implementation of NeuralNetwork constructor
NeuralNetwork::NeuralNetwork(const std::vector<size_t>& config, float mut_rate, ActivationFunc activ)
    : mutation_rate(mut_rate), activ_func(activ) {
    // Adjust configuration to include biases
    this->config = config;
    for (size_t i = 0; i < config.size() - 1; ++i) {
        this->config[i] += 1;
    }

    // Initialize weights with He initialization
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0, 1.0);

    for (size_t i = 0; i < config.size() - 1; ++i) {
        size_t rows = config[i + 1];
        size_t cols = config[i] + 1;
        Eigen::MatrixXf weight = Eigen::MatrixXf::NullaryExpr(rows, cols, [&]() {
            float uniform = dis(gen);
            float normal = std::sqrt(-2.0 * std::log(uniform)) * std::cos(2.0 * PI * uniform);
            return normal * std::sqrt(2.0f / rows);
        });
        weights.push_back(weight);
    }
}

NeuralNetwork NeuralNetwork::crossover(const NeuralNetwork& a, const NeuralNetwork& b) {
    if (a.config != b.config) {
        throw std::invalid_argument("NN configurations must match for crossover.");
    }

    std::vector<Eigen::MatrixXf> new_weights;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0, 1.0);

    for (size_t i = 0; i < a.weights.size(); ++i) {
        Eigen::MatrixXf new_weight = a.weights[i];
        for (int r = 0; r < a.weights[i].rows(); ++r) {
            for (int c = 0; c < a.weights[i].cols(); ++c) {
                new_weight(r, c) = dis(gen) < 0.5 ? a.weights[i](r, c) : b.weights[i](r, c);
            }
        }
        new_weights.push_back(new_weight);
    }

    return NeuralNetwork(a.config, a.mutation_rate, a.activ_func);
}

void NeuralNetwork::mutate() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0, 1.0);

    for (auto& weight : weights) {
        for (int r = 0; r < weight.rows(); ++r) {
            for (int c = 0; c < weight.cols(); ++c) {
                if (dis(gen) < mutation_rate) {
                    float uniform = dis(gen);
                    weight(r, c) = std::sqrt(-2.0 * std::log(uniform)) * std::cos(2.0 * PI * uniform);
                }
            }
        }
    }
}

std::vector<float> NeuralNetwork::feedForward(const std::vector<float>& inputs) {
    Eigen::VectorXf y = Eigen::Map<const Eigen::VectorXf>(inputs.data(), inputs.size());

    for (size_t i = 0; i < weights.size(); ++i) {
        // Add bias
        Eigen::VectorXf y_with_bias(y.size() + 1);
        y_with_bias.head(y.size()) = y;
        y_with_bias(y.size()) = 1.0f; // Set bias to 1
        
        y = weights[i] * y_with_bias;

        // Apply activation function
        y = y.unaryExpr([this](float val) {
            switch (activ_func) {
                case ActivationFunc::ReLU:
                    return std::max(0.0f, val);
                case ActivationFunc::Sigmoid:
                    return 1.0f / (1.0f + std::exp(-val));
                case ActivationFunc::Tanh:
                    return std::tanh(val);
            }
            return val;
        });
    }

    return std::vector<float>(y.data(), y.data() + y.size());
}

void drawText(float x, float y, const std::string& text, void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(font, c);
    }
}

void drawCircle(float x, float y, float radius, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 10; ++i) {
        float angle = i * PI / 5;
        glVertex2f(x + cos(angle) * radius, y + sin(angle) * radius);
    }
    glEnd();
}

void drawRectangle(float x, float y, float width, float height, bool filled, float r, float g, float b) {
    glColor3f(r, g, b);
    if (filled) {
        glBegin(GL_QUADS);
    } else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
}

void drawLine(float x1, float y1, float x2, float y2, float width, float r, float g, float b) {
    glColor3f(r, g, b);
    glLineWidth(width);
    glBegin(GL_LINES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glEnd();
}

void drawNeuralNetwork(
    const std::vector<size_t>& config,
    const std::vector<Eigen::MatrixXf>& weights,
    const std::vector<float>& inputs,
    const std::vector<float>& outputs,
    float width, float height, bool bias
) {
    drawRectangle(-width*0.6, -height*0.6, width*1.2, height*1.2, false, 1.0f, 1.0f, 1.0f);

    float vspace = height / (*std::max_element(config.begin(), config.end()) - 1);
    std::vector<std::pair<float, float>> prevLayerPositions;
    std::vector<std::pair<float, float>> currLayerPositions;

    for (size_t i = 0; i < config.size(); ++i) {
        size_t layerSize = config[i] - (i < config.size() - 1 && !bias ? 1 : 0);
        currLayerPositions.clear();

        for (size_t j = 0; j < layerSize; ++j) {
            float x = i * width / (config.size() - 1) - width * 0.5f;
            float y = j * vspace - (layerSize - 1) * vspace * 0.5f;
            currLayerPositions.emplace_back(x, y);
            drawCircle(x, y, 20.0f, 1.0f, 1.0f, 1.0f);
            if (i == 0 && j < inputs.size()) {
                std::string input_text = std::to_string(inputs[j]).substr(0, 4); // Format input
                drawText(x - 0.03f, y - 0.02f, input_text);
            } else if (i == config.size() - 1 && j < outputs.size()) {
                std::string output_text = std::to_string(outputs[j]).substr(0, 4); // Format output
                drawText(x + 0.02f, y, output_text);
            }
        }

        if (i > 0) {
            for (size_t k = 0; k < prevLayerPositions.size(); ++k) {
                for (size_t j = 0; j < currLayerPositions.size(); ++j) {
                    float weight = weights[i - 1](j, k);
                    float color = weight < 0.0f ? 0.0f : 1.0f;
                    drawLine(
                        prevLayerPositions[k].first, prevLayerPositions[k].second,
                        currLayerPositions[j].first, currLayerPositions[j].second,
                        1.5f, 1.0f, color, color
                    );
                }
            }
        }

        prevLayerPositions = currLayerPositions;
    }

    // Draw legend
	//    drawRectangle(width * 0.47f, height * 0.47f, 10.0f, 10.0f, true, 1.0f, 0.0f, 0.0f); // Red box for -ve
	//    drawText(width * 0.47f + 20.0f, height * 0.47f + 10.0f, "-ve");
	//
	//    drawRectangle(width * 0.47f, height * 0.47f + 20.0f, 10.0f, 10.0f, true, 1.0f, 1.0f, 1.0f); // White box for +ve
	//    drawText(width * 0.47f + 20.0f, height * 0.47f + 30.0f, "+ve");
}

// Global neural network instance for controlling the ship
NeuralNetwork* g_neuralNetwork = nullptr;

// Function to get the closest asteroid in each direction (8 sensors around the ship)
std::vector<float> getSensorInputs(const World& world) {
    std::vector<float> inputs;
    
    // Player position and velocity
    inputs.push_back(static_cast<float>(world.player.x / ORTHO_MAX)); // Normalized x position
    inputs.push_back(static_cast<float>(world.player.y / ORTHO_MAX)); // Normalized y position
    inputs.push_back(static_cast<float>(world.player.xv * 1000)); // x velocity (scaled)
    inputs.push_back(static_cast<float>(world.player.yv * 1000)); // y velocity (scaled)
    inputs.push_back(static_cast<float>(world.player.angle / 360.0)); // Normalized angle
    
    // 8 distance sensors around the ship (like rays casting in different directions)
    for (int sensor = 0; sensor < 8; ++sensor) {
        float sensorAngle = sensor * 45.0f; // 0, 45, 90, 135, 180, 225, 270, 315 degrees
        float minDistance = 1.0f; // Maximum distance (normalized)
        
        for (const auto& asteroid : world.asteroids) {
            // Calculate angle from player to asteroid
            double dx = asteroid.motion.x - world.player.x;
            double dy = asteroid.motion.y - world.player.y;
            double asteroidAngle = atan2(dy, dx) * 180.0 / PI;
            if (asteroidAngle < 0) asteroidAngle += 360.0;
            
            // Check if asteroid is within the sensor's field of view (±22.5 degrees)
            double angleDiff = fabs(asteroidAngle - sensorAngle);
            if (angleDiff > 180) angleDiff = 360 - angleDiff;
            
            if (angleDiff <= 22.5) {
                double distance = sqrt(dx*dx + dy*dy);
                double normalizedDistance = distance / (ORTHO_MAX * 0.7); // Normalize to screen diagonal
                if (normalizedDistance < minDistance) {
                    minDistance = static_cast<float>(normalizedDistance);
                }
            }
        }
        inputs.push_back(minDistance);
    }
    
    return inputs;
}

// Function to apply neural network outputs to control the ship
void applyNeuralControl(World& world, const std::vector<float>& outputs) {
    if (outputs.size() >= 4) {
        // Use sigmoid-like threshold for binary controls
        world.control.thrusting = outputs[0] > 0.5f;
        world.control.turningLeft = outputs[1] > 0.5f;
        world.control.turningRight = outputs[2] > 0.5f;
        world.control.shooting = outputs[3] > 0.5f;
    }
}

// Function to initialize a neural network for ship control
void initializeNeuralNetwork() {
    // Network configuration: 13 inputs (5 ship state + 8 distance sensors) -> 8 hidden -> 4 outputs
    std::vector<size_t> config = {13, 8, 4};
    g_neuralNetwork = new NeuralNetwork(config, 0.1f, ActivationFunc::Sigmoid);
}

// Function to control ship using neural network
void updateNeuralControl(World& world) {
    if (g_neuralNetwork && world.alive) {
        std::vector<float> inputs = getSensorInputs(world);
        std::vector<float> outputs = g_neuralNetwork->feedForward(inputs);
        applyNeuralControl(world, outputs);
    }
}

