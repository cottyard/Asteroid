#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <fstream>
#include <sstream>
#include <Eigen/Dense>
#include <GL/freeglut.h>
#include "NeuNet.h"
#include "Common.h"

// Use the PI constant from Common.h instead of defining our own

// Implementation of NeuralNetwork constructor
NeuralNetwork::NeuralNetwork(const std::vector<size_t>& config, float mut_rate)
    : mutation_rate(mut_rate) {
    // Store original config and create modified config for bias handling
    std::vector<size_t> original_config = config;
    this->config = config;
    for (size_t i = 0; i < config.size() - 1; ++i) {
        this->config[i] += 1; // Add bias to each layer except output
    }

    // Initialize weights with He initialization
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0, 1.0);

    for (size_t i = 0; i < original_config.size() - 1; ++i) {
        size_t rows = original_config[i + 1];  // Output layer size (no bias)
        size_t cols = original_config[i] + 1;  // Input layer size + bias
        Eigen::MatrixXf weight = Eigen::MatrixXf::NullaryExpr(rows, cols, [&]() {
            float uniform = dis(gen);
            float normal = std::sqrt(-2.0 * std::log(uniform)) * std::cos(2.0 * PI * uniform);
            return normal * std::sqrt(2.0f / cols);  // Use cols (fan-in) for He initialization
        });
        weights.push_back(weight);
    }
}

NeuralNetwork NeuralNetwork::crossover(const NeuralNetwork& a, const NeuralNetwork& b) {
    if (a.config != b.config) {
        throw std::invalid_argument("NN configurations must match for crossover.");
    }

    // Create offspring with same configuration but different weights
    std::vector<size_t> original_config;
    for (size_t i = 0; i < a.config.size(); ++i) {
        if (i < a.config.size() - 1) {
            original_config.push_back(a.config[i] - 1); // Remove bias
        } else {
            original_config.push_back(a.config[i]); // Output layer has no bias
        }
    }
    
    NeuralNetwork offspring(original_config, a.mutation_rate);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0, 1.0);

    for (size_t i = 0; i < a.weights.size(); ++i) {
        for (int r = 0; r < a.weights[i].rows(); ++r) {
            for (int c = 0; c < a.weights[i].cols(); ++c) {
                offspring.weights[i](r, c) = dis(gen) < 0.5 ? a.weights[i](r, c) : b.weights[i](r, c);
            }
        }
    }

    return offspring;
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

        // Apply activation function: tanh for first hidden, ReLU for second hidden, Sigmoid for output
        bool is_output_layer = (i == weights.size() - 1);
        bool is_first_hidden = (i == 0);
        
        if (is_output_layer) {
            // Sigmoid for output layer (for binary control outputs)
            y = y.unaryExpr([](float val) {
                return 1.0f / (1.0f + std::exp(-val));
            });
        } else if (is_first_hidden) {
            // Tanh for first hidden layer (handles negative inputs like velocity/angle)
            y = y.unaryExpr([](float val) {
                return std::tanh(val);
            });
        } else {
            // ReLU for second hidden layer
            y = y.unaryExpr([](float val) {
                return std::max(0.0f, val);
            });
        }
    }

    return std::vector<float>(y.data(), y.data() + y.size());
}

void drawText(float x, float y, const std::string& text, void* font = GLUT_BITMAP_HELVETICA_18) {
    glColor3f(0.0f, 1.0f, 1.0f); // Bright cyan color for text visibility
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(font, c);
    }
}

void drawTextWithColor(float x, float y, const std::string& text, float r, float g, float b, void* font = GLUT_BITMAP_HELVETICA_12) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(font, c);
    }
}

void drawCircle(float x, float y, float radius, float r, float g, float b, bool filled = false) {
    glColor3f(r, g, b);
    if (filled) {
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(x, y); // Center vertex
        for (int i = 0; i <= 32; ++i) { // 32 segments for smooth circle
            float angle = i * 2.0f * PI / 32;
            glVertex2f(x + cos(angle) * radius, y + sin(angle) * radius);
        }
    } else {
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 32; ++i) { // 32 segments for smooth circle
            float angle = i * 2.0f * PI / 32;
            glVertex2f(x + cos(angle) * radius, y + sin(angle) * radius);
        }
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
    drawRectangle(-width*0.7, -height*0.6, width*1.4, height*1.2, false, 1.0f, 1.0f, 1.0f);

    float vspace = height / (*std::max_element(config.begin(), config.end()) - 1);
    std::vector<std::pair<float, float>> prevLayerPositions;
    std::vector<std::pair<float, float>> currLayerPositions;
    std::vector<std::vector<float>> layerActivations; // Store activations for each layer

    // Calculate activations for each layer (simulate forward pass)
    std::vector<float> current_activations = inputs;
    layerActivations.push_back(current_activations);
    
    // Forward pass through the network to get intermediate activations
    for (size_t layer = 0; layer < weights.size(); ++layer) {
        std::vector<float> next_activations;
        
        for (int neuron = 0; neuron < weights[layer].rows(); ++neuron) {
            float sum = 0.0f;
            for (int input_idx = 0; input_idx < weights[layer].cols() - 1; ++input_idx) {
                if (input_idx < current_activations.size()) {
                    sum += weights[layer](neuron, input_idx) * current_activations[input_idx];
                }
            }
            sum += weights[layer](neuron, weights[layer].cols() - 1); // Add bias
            
            // Apply appropriate activation: tanh for first hidden, ReLU for second hidden, Sigmoid for output
            float activation;
            bool is_output_layer = (layer == weights.size() - 1);
            bool is_first_hidden = (layer == 0);
            
            if (is_output_layer) {
                activation = 1.0f / (1.0f + std::exp(-sum)); // Sigmoid for output
            } else if (is_first_hidden) {
                activation = std::tanh(sum); // Tanh for first hidden layer
            } else {
                activation = std::max(0.0f, sum); // ReLU for second hidden layer
            }
            next_activations.push_back(activation);
        }
        
        current_activations = next_activations;
        layerActivations.push_back(current_activations);
    }

    for (size_t i = 0; i < config.size(); ++i) {
        size_t layerSize = config[i] - (i < config.size() - 1 && !bias ? 1 : 0);
        currLayerPositions.clear();

        // Add layer titles
        float layer_x = i * width / (config.size() - 1) - width * 0.5f;
        std::string layer_title;
        if (i == 0) {
            layer_title = "INPUT LAYER";
        } else if (i == config.size() - 1) {
            layer_title = "OUTPUT LAYER";
        } else {
            layer_title = "HIDDEN " + std::to_string(i);
        }
        drawTextWithColor(layer_x - 30.0f, height * 0.4f, layer_title, 1.0f, 1.0f, 1.0f); // White

        for (size_t j = 0; j < layerSize; ++j) {
            float x = i * width / (config.size() - 1) - width * 0.5f;
            float y = j * vspace - (layerSize - 1) * vspace * 0.5f;
            currLayerPositions.emplace_back(x, y);
            
            // Determine neuron color and visualization based on activation
            bool isActivated = false;
            float activationValue = 0.0f;
            float r = 0.7f, g = 0.7f, b = 0.7f; // Default gray
            
            if (i < layerActivations.size() && j < layerActivations[i].size()) {
                activationValue = layerActivations[i][j];
                bool is_output_layer = (i == config.size() - 1);
                
                // Consider neuron activated if value > threshold
                isActivated = is_output_layer ? (activationValue > 0.5f) : (activationValue > 0.1f);
                
                // Color-code neurons by activation strength
                if (isActivated) {
                    // Green intensity based on activation value
                    float intensity = is_output_layer ? activationValue : std::min(1.0f, activationValue / 2.0f);
                    r = 0.0f;
                    g = 0.5f + 0.5f * intensity; // Range from 0.5 to 1.0
                    b = 0.0f;
                } else {
                    // Dim red/gray for low activation
                    float dim_factor = is_output_layer ? activationValue * 0.5f : std::min(0.3f, activationValue * 0.3f);
                    r = 0.3f + dim_factor;
                    g = 0.3f + dim_factor;
                    b = 0.3f + dim_factor;
                }
            }
            
            // Draw neuron with improved visualization
            if (isActivated) {
                // Filled circle for activated neurons
                drawCircle(x, y, 20.0f, r, g, b, true);
                // White outline
                drawCircle(x, y, 20.0f, 1.0f, 1.0f, 1.0f, false);
            } else {
                // Just outline for inactive neurons
                drawCircle(x, y, 20.0f, r, g, b, false);
            }
            
            // Draw activation value as text on the neuron
            if (i < layerActivations.size() && j < layerActivations[i].size()) {
                std::string activation_text;
                if (activationValue < 0.01f && activationValue > -0.01f) {
                    activation_text = "0.0";
                } else {
                    activation_text = std::to_string(activationValue).substr(0, 4);
                }
                
                // Choose text color for contrast
                float text_r = (r + g + b) / 3.0f > 0.5f ? 0.0f : 1.0f;
                float text_g = (r + g + b) / 3.0f > 0.5f ? 0.0f : 1.0f;
                float text_b = (r + g + b) / 3.0f > 0.5f ? 0.0f : 1.0f;
                
                drawTextWithColor(x - 12.0f, y - 3.0f, activation_text, text_r, text_g, text_b);
            }
            
            // Draw input/output labels with descriptive names
            if (i == 0 && j < inputs.size()) {
                std::string input_label;
                std::string input_value = std::to_string(inputs[j]).substr(0, 4);
                
                // Define input labels based on getSensorInputs structure
                switch (j) {
                    case 0: input_label = "X Pos: " + input_value; break;
                    case 1: input_label = "Y Pos: " + input_value; break;
                    case 2: input_label = "X Vel: " + input_value; break;
                    case 3: input_label = "Y Vel: " + input_value; break;
                    case 4: input_label = "Angle: " + input_value; break;
                    // 8 distance sensors (0°=East, 45°=NE, 90°=North, 135°=NW, 180°=West, 225°=SW, 270°=South, 315°=SE)
                    case 5: input_label = "Dist E: " + input_value; break;
                    case 6: input_label = "Dist NE: " + input_value; break;
                    case 7: input_label = "Dist N: " + input_value; break;
                    case 8: input_label = "Dist NW: " + input_value; break;
                    case 9: input_label = "Dist W: " + input_value; break;
                    case 10: input_label = "Dist SW: " + input_value; break;
                    case 11: input_label = "Dist S: " + input_value; break;
                    case 12: input_label = "Dist SE: " + input_value; break;
                    default: input_label = "In" + std::to_string(j) + ": " + input_value; break;
                }
                drawTextWithColor(x - 180.0f, y - 5.0f, input_label, 0.0f, 1.0f, 1.0f); // Cyan
            } else if (i == config.size() - 1 && j < outputs.size()) {
                std::string output_label;
                std::string output_value = std::to_string(outputs[j]).substr(0, 4);
                
                // Define output labels based on applyNeuralControl structure
                switch (j) {
                    case 0: output_label = "Thrust: " + output_value; break;
                    case 1: output_label = "Turn L: " + output_value; break;
                    case 2: output_label = "Turn R: " + output_value; break;
                    case 3: output_label = "Shoot: " + output_value; break;
                    default: output_label = "Out" + std::to_string(j) + ": " + output_value; break;
                }
                drawTextWithColor(x + 40.0f, y - 5.0f, output_label, 1.0f, 1.0f, 0.0f); // Yellow
            }
        }

        // Draw connections with improved weight-based visualization
        if (i > 0) {
            for (size_t k = 0; k < prevLayerPositions.size(); ++k) {
                for (size_t j = 0; j < currLayerPositions.size(); ++j) {
                    // Get the actual weight value from the network (this is FIXED for each individual)
                    float weight = weights[i - 1](j, k);
                    
                    // Calculate weight magnitude and normalize
                    float weight_magnitude = std::abs(weight);
                    float max_weight = 4.0f; // Normalization factor
                    float normalized_magnitude = std::min(1.0f, weight_magnitude / max_weight);
                    
                    // Skip drawing very weak connections to reduce visual clutter
                    if (weight_magnitude < 0.05f) continue;
                    
                    // Calculate line thickness based on weight magnitude (0.5 to 4.0 pixels)
                    float line_thickness = 0.5f + normalized_magnitude * 3.5f;
                    
                    // Calculate brightness for better visibility (0.3 to 1.0)
                    float brightness = 0.3f + normalized_magnitude * 0.7f;
                    
                    // Color coding based on weight sign
                    float r, g, b;
                    if (weight < 0.0f) {
                        // Negative weights: Red spectrum with brightness based on magnitude
                        r = brightness;
                        g = 0.1f;
                        b = 0.1f;
                    } else {
                        // Positive weights: Green spectrum with brightness based on magnitude
                        r = 0.1f;
                        g = brightness;
                        b = 0.1f;
                    }
                    
                    // Add blue tint for very strong weights (magnitude > 80% of max)
                    if (normalized_magnitude > 0.8f) {
                        b = 0.4f;
                    }
                    
                    drawLine(
                        prevLayerPositions[k].first, prevLayerPositions[k].second,
                        currLayerPositions[j].first, currLayerPositions[j].second,
                        line_thickness, r, g, b
                    );
                }
            }
        }

        prevLayerPositions = currLayerPositions;
    }
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
    // Network configuration: 13 inputs (5 ship state + 8 distance sensors) -> 8 hidden -> 8 hidden -> 4 outputs
    // This matches the genetic algorithm configuration exactly
    std::vector<size_t> config = {13, 8, 8, 4};
    g_neuralNetwork = new NeuralNetwork(config, 0.1f);
}

// Function to control ship using neural network
void updateNeuralControl(World& world) {
    if (g_neuralNetwork && world.alive) {
        std::vector<float> inputs = getSensorInputs(world);
        std::vector<float> outputs = g_neuralNetwork->feedForward(inputs);
        applyNeuralControl(world, outputs);
    }
}

// Function to load a neural network from file for testing
bool loadNeuralNetworkFromFile(const std::string& filename) {
    try {
        // Delete existing neural network
        if (g_neuralNetwork) {
            delete g_neuralNetwork;
            g_neuralNetwork = nullptr;
        }
        
        // Load new neural network from file
        NeuralNetwork loaded_nn = NeuralNetwork::loadFromFile(filename);
        g_neuralNetwork = new NeuralNetwork(loaded_nn);
        
        std::cout << "Successfully loaded neural network from " << filename << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to load neural network from " << filename << ": " << e.what() << std::endl;
        
        // Fall back to default neural network
        initializeNeuralNetwork();
        return false;
    }
}

// JSON save/load implementation
void NeuralNetwork::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing." << std::endl;
        return;
    }
    file << toJson();
    file.close();
}

NeuralNetwork NeuralNetwork::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for reading." << std::endl;
        throw std::runtime_error("Could not open file: " + filename);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    return fromJson(buffer.str());
}

std::string NeuralNetwork::toJson() const {
    std::ostringstream json;
    json << "{";
    
    // Convert config back to original (remove bias)
    json << "\"config\":[";
    for (size_t i = 0; i < config.size(); ++i) {
        if (i > 0) json << ",";
        if (i < config.size() - 1) {
            json << (config[i] - 1); // Remove bias from hidden layers
        } else {
            json << config[i]; // Output layer has no bias
        }
    }
    json << "],";
    
    // Serialize weights
    json << "\"weights\":[";
    for (size_t w = 0; w < weights.size(); ++w) {
        if (w > 0) json << ",";
        json << "[";
        
        // Flatten the matrix
        json << "[";
        for (int i = 0; i < weights[w].rows(); ++i) {
            for (int j = 0; j < weights[w].cols(); ++j) {
                if (i > 0 || j > 0) json << ",";
                json << weights[w](i, j);
            }
        }
        json << "],";
        
        // Add dimensions
        json << weights[w].rows() << "," << weights[w].cols();
        json << "]";
    }
    json << "],";
    
    // Mutation rate
    json << "\"mut_rate\":" << mutation_rate;
    
    json << "}";
    return json.str();
}

NeuralNetwork NeuralNetwork::fromJson(const std::string& json) {
    // Simple JSON parser (for this specific format)
    std::string json_str = json;
    
    // Extract config
    std::vector<size_t> config;
    size_t config_start = json_str.find("\"config\":[") + 10;
    size_t config_end = json_str.find("]", config_start);
    std::string config_str = json_str.substr(config_start, config_end - config_start);
    
    std::istringstream config_stream(config_str);
    std::string token;
    while (std::getline(config_stream, token, ',')) {
        config.push_back(std::stoi(token));
    }
    
    // Extract mutation rate
    size_t mut_start = json_str.find("\"mut_rate\":") + 11;
    size_t mut_end = json_str.find("}", mut_start);
    std::string mut_str = json_str.substr(mut_start, mut_end - mut_start);
    float mut_rate = std::stof(mut_str);
    
    // Create the neural network (no activation function parameter needed)
    NeuralNetwork nn(config, mut_rate);
    
    // Extract and set weights
    size_t weights_start = json_str.find("\"weights\":[") + 11;
    size_t weights_end = json_str.find("],\"activ_func\"");
    if (weights_end == std::string::npos) {
        weights_end = json_str.find("],\"mut_rate\""); // Handle new format
    }
    std::string weights_section = json_str.substr(weights_start, weights_end - weights_start);
    
    // Parse weights (simplified parser for the specific format)
    nn.weights.clear();
    size_t pos = 0;
    size_t weight_idx = 0;
    
    while ((pos = weights_section.find("[[", pos)) != std::string::npos && weight_idx < config.size() - 1) {
        pos += 2; // Skip "[[
        
        // Find the end of the weight array
        size_t array_end = weights_section.find("],", pos);
        std::string weight_data = weights_section.substr(pos, array_end - pos);
        
        // Find dimensions after the array
        size_t dim_start = array_end + 2;
        size_t dim_end = weights_section.find("]", dim_start);
        std::string dims = weights_section.substr(dim_start, dim_end - dim_start);
        
        // Parse dimensions
        std::istringstream dim_stream(dims);
        std::string dim_token;
        std::getline(dim_stream, dim_token, ',');
        int rows = std::stoi(dim_token);
        std::getline(dim_stream, dim_token, ',');
        int cols = std::stoi(dim_token);
        
        // Parse weight values
        std::vector<float> values;
        std::istringstream value_stream(weight_data);
        std::string value_token;
        while (std::getline(value_stream, value_token, ',')) {
            values.push_back(std::stof(value_token));
        }
        
        // Create weight matrix
        Eigen::MatrixXf weight_matrix(rows, cols);
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                weight_matrix(i, j) = values[i * cols + j];
            }
        }
        
        nn.weights.push_back(weight_matrix);
        weight_idx++;
        pos = dim_end;
    }
    
    return nn;
}

