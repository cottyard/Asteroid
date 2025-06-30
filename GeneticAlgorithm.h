#pragma once

#include "NeuNet.h"
#include "Common.h"
#include <vector>
#include <memory>
#include <random>

struct Individual {
    NeuralNetwork* brain;
    World world;
    float fitness;
    bool alive;
    int lifespan;
    int shots_fired;
    int asteroids_destroyed;
    
    Individual(const std::vector<size_t>& config, float mut_rate);
    Individual(const Individual& other);
    Individual& operator=(const Individual& other);
    ~Individual() { delete brain; }
    
    void reset();
    void update();
    void calculateFitness();
};

class Population {
public:
    // Genetic algorithm parameters
    static const size_t ELITE_COUNT = 10; // Keep top 10 individuals
    static const int MAX_LIFESPAN = 30000; // Maximum time an individual can live (in frames)

private:
    std::vector<Individual*> individuals;
    size_t population_size;
    size_t current_generation;
    size_t tracked_individual;
    std::vector<size_t> network_config;
    float mutation_rate;
    
    mutable std::mt19937 rng;
    mutable std::uniform_real_distribution<float> uniform_dist;
    
public:
    Population(size_t pop_size, const std::vector<size_t>& config, float mut_rate);
    ~Population();
    
    void update();
    void nextGeneration();
    bool isGenerationComplete() const;
    
    // Selection methods
    std::vector<size_t> selectParents() const;
    Individual* tournamentSelection() const;
    Individual* rouletteWheelSelection() const;
    
    // Getters
    size_t getCurrentGeneration() const { return current_generation; }
    size_t getPopulationSize() const { return population_size; }
    Individual* getTrackedIndividual() { return individuals[tracked_individual]; }
    size_t getTrackedIndex() const { return tracked_individual; }
    Individual* getBestIndividual();
    float getBestFitness() const;
    float getAverageFitness() const;
    
    // Control methods
    void setTrackedIndividual(size_t index);
    void trackBestAlive();
    void trackBest();
    
    // Statistics
    void printGenerationStats() const;
    
    // Elite management
    void saveElites() const;
    bool loadElites();
    
    // Training control
    bool shouldContinueTraining() const { return current_generation < 10; }
};

// Global population instance
extern Population* g_population;

// Training control functions
void initializeGeneticTraining();
void updateGeneticTraining();
void toggleGeneticTraining();
bool isGeneticTrainingActive();

// External function declarations
void stepWorld(World& world, int delta);
std::vector<float> getSensorInputs(const World& world);
void applyNeuralControl(World& world, const std::vector<float>& outputs);

// Statistics and display
void displayTrainingStats();
void displayNeuralNetworkVisualization();
