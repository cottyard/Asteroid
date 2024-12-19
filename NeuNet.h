//#pragma once
//
//#include <vector>
//#include <Eigen/Dense>
//#include <random>
//#include <string>
//
//enum class ActivationFunc {
//    ReLU,
//    Sigmoid,
//    Tanh
//};
//
//class NN {
//public:
//    NN(const std::vector<size_t>& config, float mutRate, ActivationFunc activ);
//
//    static NN crossover(const NN& a, const NN& b);
//    void mutate();
//    std::vector<float> feedForward(const std::vector<float>& inputs) const;
//
//    void draw(float width, float height, const std::vector<float>& inputs, const std::vector<float>& outputs, bool bias) const;
//
//    std::string exportToJSON() const;
//    static NN importFromJSON(const std::string& filePath);
//
//private:
//    std::vector<size_t> config;
//    std::vector<Eigen::MatrixXf> weights;
//    ActivationFunc activFunc;
//    float mutRate;
//
//    float randomNormal() const;
//    Eigen::MatrixXf initWeights(size_t rows, size_t cols, size_t fanIn) const;
//};

