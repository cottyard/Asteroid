#include "GeneticAlgorithm.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <chrono>
#include <fstream>
#include <fstream>

// Global population instance
Population* g_population = nullptr;
bool genetic_training_active = false;

// External declarations
void stepWorld(World& world, int delta);

// Static member definitions
const size_t Population::ELITE_COUNT;

// Individual implementation
Individual::Individual(const std::vector<size_t>& config, float mut_rate)
    : brain(new NeuralNetwork(config, mut_rate))
    , fitness(0.0f)
    , alive(true)
    , lifespan(0)
    , shots_fired(0)
    , asteroids_destroyed(0) {
    reset();
}

Individual::Individual(const Individual& other)
    : brain(new NeuralNetwork(*other.brain))
    , world(other.world)
    , fitness(other.fitness)
    , alive(other.alive)
    , lifespan(other.lifespan)
    , shots_fired(other.shots_fired)
    , asteroids_destroyed(other.asteroids_destroyed) {
}

Individual& Individual::operator=(const Individual& other) {
    if (this != &other) {
        delete brain;
        brain = new NeuralNetwork(*other.brain);
        world = other.world;
        fitness = other.fitness;
        alive = other.alive;
        lifespan = other.lifespan;
        shots_fired = other.shots_fired;
        asteroids_destroyed = other.asteroids_destroyed;
    }
    return *this;
}

void Individual::reset() {
    // Initialize world state
    world.score = 0;
    world.spawnTimer = 0;
    world.shootTimer = 0;
    world.alive = true;
    world.control = {false, false, false, false};
    
    // Initialize player position at center of the screen (like the normal game)
    world.player.x = ORTHO_MAX / 2;
    world.player.y = ORTHO_MAX / 2;
    world.player.xv = 0.0;
    world.player.yv = 0.0;
    world.player.angle = 90.0; // Start facing up like the normal game
    world.player.angularVelocity = 0.0;
    
    // Clear bullets and asteroids
    world.bullets.clear();
    world.asteroids.clear();
    
    // Spawn initial asteroids
    for (int i = 0; i < 5; ++i) {
        Asteroid asteroid;
        asteroid.radius = rand() % 3 + 3; // Use the same radius as normal game (3-5)
        
        // Spawn asteroids away from player (centered at ORTHO_MAX/2)
        float angle = (i * 360.0f / 5.0f) * PI / 180.0f;
        float distance = 20.0f + (rand() % 15); // Closer to player but still safe distance
        asteroid.motion.x = (ORTHO_MAX / 2) + distance * cos(angle);
        asteroid.motion.y = (ORTHO_MAX / 2) + distance * sin(angle);
        
        // Random velocity (similar to normal game)
        int direction = rand() % 360;
        double speed = (rand() % 30 + 100) / 10000.0; // Same speed as normal asteroids
        asteroid.motion.xv = speed * cos(direction * PI / 180);
        asteroid.motion.yv = speed * sin(direction * PI / 180);
        asteroid.motion.angle = rand() % 360;
        asteroid.motion.angularVelocity = (rand() % 21 - 10) / 100.0; // Same as normal asteroids
        
        world.asteroids.push_back(asteroid);
    }
    
    // Reset individual stats
    fitness = 0.0f;
    alive = true;
    lifespan = 0;
    shots_fired = 0;
    asteroids_destroyed = 0;
}

void Individual::update() {
    if (!alive || !world.alive) return;
    
    lifespan++;
    
    // Get neural network inputs
    std::vector<float> inputs = getSensorInputs(world);
    
    // Get neural network outputs
    std::vector<float> outputs = brain->feedForward(inputs);
    
    // Apply outputs to control
    applyNeuralControl(world, outputs);
    
    // Track shots fired
    if (world.control.shooting && world.shootTimer <= 0) {
        shots_fired++;
    }
    
    // Store previous asteroid count
    size_t prev_asteroid_count = world.asteroids.size();
    
    // Update world simulation
    stepWorld(world, 16); // Assuming 60 FPS (16ms per frame)
    
    // Track asteroids destroyed
    if (world.asteroids.size() < prev_asteroid_count) {
        asteroids_destroyed += prev_asteroid_count - world.asteroids.size();
    }
    
    // Check if individual should die
    if (!world.alive) {
        alive = false;
        calculateFitness();
    } else if (lifespan >= Population::MAX_LIFESPAN) {
        alive = false;
        world.alive = false;
        calculateFitness();
    }
}

void Individual::calculateFitness() {
    if (fitness > 0) return; // Already calculated
    
    // Fitness components:
    // 1. Survival time (encourages staying alive longer)
    float survival_bonus = static_cast<float>(lifespan) / Population::MAX_LIFESPAN * 100.0f;
    
    // 2. Score from destroying asteroids
    float score_bonus = static_cast<float>(world.score) * 10.0f;
    
    // 3. Asteroids destroyed directly
    float destruction_bonus = static_cast<float>(asteroids_destroyed) * 50.0f;
    
    // 4. Penalty for excessive shooting (encourage efficient shooting)
    float shooting_penalty = 0.0f;
    if (shots_fired > 0) {
        float accuracy = static_cast<float>(asteroids_destroyed) / static_cast<float>(shots_fired);
        shooting_penalty = shots_fired * (1.0f - accuracy) * 0.1f;
    }
    
    // 5. Bonus for movement (encourage active behavior)
    float movement_bonus = std::min(50.0f, static_cast<float>(lifespan) * 0.01f);
    
    fitness = survival_bonus + score_bonus + destruction_bonus + movement_bonus - shooting_penalty;
    fitness = std::max(0.1f, fitness); // Ensure minimum fitness
}

// Population implementation
Population::Population(size_t pop_size, const std::vector<size_t>& config, float mut_rate)
    : population_size(pop_size)
    , current_generation(0)
    , tracked_individual(0)
    , network_config(config)
    , mutation_rate(mut_rate)
    , rng(std::chrono::steady_clock::now().time_since_epoch().count())
    , uniform_dist(0.0f, 1.0f) {
    
    individuals.reserve(population_size);
    for (size_t i = 0; i < population_size; ++i) {
        individuals.push_back(new Individual(config, mut_rate));
    }
    
    std::cout << "Initialized population with " << population_size << " individuals for generation " << current_generation << std::endl;
}

Population::~Population() {
    for (size_t i = 0; i < individuals.size(); ++i) {
        delete individuals[i];
    }
}

void Population::update() {
    for (size_t i = 0; i < individuals.size(); ++i) {
        individuals[i]->update();
    }
    
    // Auto-switch to best alive individual if current tracked individual dies
    if (!individuals[tracked_individual]->alive) {
        trackBestAlive();
    }
}

bool Population::isGenerationComplete() const {
    for (size_t i = 0; i < individuals.size(); ++i) {
        if (individuals[i]->alive) {
            return false;
        }
    }
    return true;
}

void Population::nextGeneration() {
    if (!isGenerationComplete()) return;
    
    current_generation++;
    
    // Calculate fitness for all individuals
    for (size_t i = 0; i < individuals.size(); ++i) {
        individuals[i]->calculateFitness();
    }
    
    // Sort by fitness (descending) using a comparison function
    std::sort(individuals.begin(), individuals.end(), [](Individual* a, Individual* b) {
        return a->fitness > b->fitness;
    });
    
    printGenerationStats();
    
    if (current_generation >= 10) {
        std::cout << "Training complete after 10 generations!" << std::endl;
        std::cout << "Best individual achieved fitness: " << getBestFitness() << std::endl;
        
        // Save the top 10 elite individuals from final generation
        saveElites();
        return;
    }
    
    // Create new generation
    std::vector<Individual*> new_generation;
    new_generation.reserve(population_size);
    
    // Keep elite individuals (top 10)
    size_t elite_count = std::min(ELITE_COUNT, population_size);
    
    for (size_t i = 0; i < elite_count; ++i) {
        new_generation.push_back(new Individual(*individuals[i]));
        new_generation.back()->reset();
    }
    
    // Generate offspring through crossover and mutation
    while (new_generation.size() < population_size) {
        // Select two parents
        Individual* parent1 = tournamentSelection();
        Individual* parent2 = tournamentSelection();
        
        // Create offspring through crossover
        Individual* offspring = new Individual(network_config, mutation_rate);
        delete offspring->brain;
        offspring->brain = new NeuralNetwork(NeuralNetwork::crossover(*parent1->brain, *parent2->brain));
        
        // Mutate offspring
        offspring->brain->mutate();
        offspring->reset();
        
        new_generation.push_back(offspring);
    }
    
    // Clean up old generation
    for (size_t i = 0; i < individuals.size(); ++i) {
        delete individuals[i];
    }
    
    // Replace old generation
    individuals = new_generation;
    tracked_individual = 0; // Track the best from previous generation
    
    std::cout << "Generation " << current_generation << " created with " << individuals.size() << " individuals" << std::endl;
}

Individual* Population::tournamentSelection() const {
    const size_t tournament_size = 3;
    Individual* best = nullptr;
    
    for (size_t i = 0; i < tournament_size; ++i) {
        size_t index = rng() % individuals.size();
        if (!best || individuals[index]->fitness > best->fitness) {
            best = individuals[index];
        }
    }
    
    return best;
}

Individual* Population::rouletteWheelSelection() const {
    float total_fitness = 0.0f;
    for (size_t i = 0; i < individuals.size(); ++i) {
        total_fitness += individuals[i]->fitness;
    }
    
    if (total_fitness <= 0) {
        return individuals[rng() % individuals.size()];
    }
    
    float random_value = uniform_dist(rng) * total_fitness;
    float running_sum = 0.0f;
    
    for (size_t i = 0; i < individuals.size(); ++i) {
        running_sum += individuals[i]->fitness;
        if (running_sum >= random_value) {
            return individuals[i];
        }
    }
    
    return individuals.back(); // Fallback
}

Individual* Population::getBestIndividual() {
    Individual* best = individuals[0];
    for (size_t i = 1; i < individuals.size(); ++i) {
        if (individuals[i]->fitness > best->fitness) {
            best = individuals[i];
        }
    }
    return best;
}

float Population::getBestFitness() const {
    float best = individuals[0]->fitness;
    for (size_t i = 1; i < individuals.size(); ++i) {
        if (individuals[i]->fitness > best) {
            best = individuals[i]->fitness;
        }
    }
    return best;
}

float Population::getAverageFitness() const {
    float total = 0.0f;
    for (size_t i = 0; i < individuals.size(); ++i) {
        total += individuals[i]->fitness;
    }
    return total / individuals.size();
}

void Population::setTrackedIndividual(size_t index) {
    if (index < individuals.size()) {
        tracked_individual = index;
    }
}

void Population::trackBestAlive() {
    size_t best_index = 0;
    float best_fitness = -1.0f;
    int best_lifespan = -1;
    
    for (size_t i = 0; i < individuals.size(); ++i) {
        if (individuals[i]->alive) {
            // Prefer higher fitness, but if fitness is equal (often 0 early on), prefer longer lifespan
            if (individuals[i]->fitness > best_fitness || 
                (individuals[i]->fitness == best_fitness && individuals[i]->lifespan > best_lifespan)) {
                best_fitness = individuals[i]->fitness;
                best_lifespan = individuals[i]->lifespan;
                best_index = i;
            }
        }
    }
    
    tracked_individual = best_index;
}

void Population::trackBest() {
    size_t best_index = 0;
    for (size_t i = 1; i < individuals.size(); ++i) {
        if (individuals[i]->fitness > individuals[best_index]->fitness) {
            best_index = i;
        }
    }
    tracked_individual = best_index;
}

void Population::printGenerationStats() const {
    float best_fitness = getBestFitness();
    float avg_fitness = getAverageFitness();
    
    // Count alive individuals
    size_t alive_count = 0;
    for (size_t i = 0; i < individuals.size(); ++i) {
        if (individuals[i]->alive) {
            alive_count++;
        }
    }
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "=== Generation " << current_generation << " Stats ===" << std::endl;
    std::cout << "Best Fitness: " << best_fitness << std::endl;
    std::cout << "Average Fitness: " << avg_fitness << std::endl;
    std::cout << "Alive Individuals: " << alive_count << "/" << individuals.size() << std::endl;
    
    // Best individual stats
    Individual* best_ind = const_cast<Population*>(this)->getBestIndividual();
    std::cout << "Best Individual - Score: " << best_ind->world.score 
              << ", Lifespan: " << best_ind->lifespan
              << ", Asteroids Destroyed: " << best_ind->asteroids_destroyed << std::endl;
    std::cout << "=============================" << std::endl;
}

// Elite management functions
void Population::saveElites() const {
    std::cout << "Saving top " << ELITE_COUNT << " elite individuals..." << std::endl;
    
    for (size_t i = 0; i < std::min(ELITE_COUNT, individuals.size()); ++i) {
        std::string filename = "brain/elite_" + std::to_string(i + 1) + ".json";
        try {
            individuals[i]->brain->saveToFile(filename);
            std::cout << "Saved elite #" << (i + 1) << " with fitness " << individuals[i]->fitness 
                      << " to " << filename << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Failed to save elite #" << (i + 1) << ": " << e.what() << std::endl;
        }
    }
}

bool Population::loadElites() {
    std::cout << "Loading previously saved elites..." << std::endl;
    
    size_t loaded_count = 0;
    for (size_t i = 0; i < ELITE_COUNT && i < individuals.size(); ++i) {
        std::string filename = "brain/elite_" + std::to_string(i + 1) + ".json";
        std::ifstream file(filename);
        
        if (file.good()) {
            file.close();
            try {
                NeuralNetwork loaded_nn = NeuralNetwork::loadFromFile(filename);
                delete individuals[i]->brain;
                individuals[i]->brain = new NeuralNetwork(loaded_nn);
                individuals[i]->reset();
                loaded_count++;
                std::cout << "Loaded elite #" << (i + 1) << " from " << filename << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Failed to load " << filename << ": " << e.what() << std::endl;
            }
        }
    }
    
    if (loaded_count > 0) {
        std::cout << "Successfully loaded " << loaded_count << " elite individuals from previous training." << std::endl;
        return true;
    } else {
        std::cout << "No elite individuals found. Starting with random population." << std::endl;
        return false;
    }
}

// Global training control functions
void initializeGeneticTraining() {
    const size_t population_size = 50; // Start with smaller population for faster training
    const std::vector<size_t> network_config = {13, 8, 8, 4}; // Input, Hidden1, Hidden2, Output
    const float mutation_rate = 0.1f;
    
    delete g_population; // Clean up any existing population
    g_population = new Population(population_size, network_config, mutation_rate);
    
    // Try to load previous elites
    g_population->loadElites();
    
    genetic_training_active = true;
    
    std::cout << "Genetic algorithm training started!" << std::endl;
    std::cout << "Population size: " << population_size << std::endl;
    std::cout << "Network architecture: ";
    for (size_t i = 0; i < network_config.size(); ++i) {
        std::cout << network_config[i];
        if (i < network_config.size() - 1) std::cout << " -> ";
    }
    std::cout << std::endl;
    std::cout << "Target generations: 10" << std::endl;
}

void updateGeneticTraining() {
    if (!genetic_training_active || !g_population) return;
    
    g_population->update();
    
    if (g_population->isGenerationComplete()) {
        g_population->nextGeneration();
        
        if (!g_population->shouldContinueTraining()) {
            genetic_training_active = false;
            std::cout << "Training completed!" << std::endl;
        }
    }
}

void toggleGeneticTraining() {
    genetic_training_active = !genetic_training_active;
    
    if (genetic_training_active && !g_population) {
        initializeGeneticTraining();
    }
    
    std::cout << "Genetic training " << (genetic_training_active ? "enabled" : "disabled") << std::endl;
}

bool isGeneticTrainingActive() {
    return genetic_training_active && g_population != nullptr;
}
