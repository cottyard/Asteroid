#pragma once

#include <vector>
#include <Eigen/Dense>
#include <random>
#include <string>

// Forward declaration
struct World;

enum class ActivationFunc {
    ReLU,
    Sigmoid,
    Tanh
};

class NeuralNetwork {
public:
    std::vector<size_t> config;
    std::vector<Eigen::MatrixXf> weights;
    ActivationFunc activ_func;
    float mutation_rate;

    NeuralNetwork(const std::vector<size_t>& config, float mut_rate, ActivationFunc activ);

    static NeuralNetwork crossover(const NeuralNetwork& a, const NeuralNetwork& b);
    void mutate();
    std::vector<float> feedForward(const std::vector<float>& inputs);

    void draw(float width, float height, const std::vector<float>& inputs, const std::vector<float>& outputs, bool bias) const;

private:
    float randomNormal() const;
    Eigen::MatrixXf initWeights(size_t rows, size_t cols, size_t fanIn) const;
};

// Function to get sensor inputs for the neural network
std::vector<float> getSensorInputs(const World& world);

// Function to apply neural network outputs to control the ship
void applyNeuralControl(World& world, const std::vector<float>& outputs);

// Function to initialize a neural network for ship control
void initializeNeuralNetwork();

// Function to control ship using neural network
void updateNeuralControl(World& world);

// Global neural network instance
extern NeuralNetwork* g_neuralNetwork;

