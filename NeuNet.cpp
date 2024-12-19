#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <Eigen/Dense>
//#include <string>
//#include <iostream>
#include <GL/freeglut.h>
//#include <nlohmann/json.hpp>

constexpr float PI = 3.14159265358979323846;

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

    NeuralNetwork(const std::vector<size_t>& config, float mut_rate, ActivationFunc activ)
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

    static NeuralNetwork crossover(const NeuralNetwork& a, const NeuralNetwork& b) {
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

    void mutate() {
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

    std::vector<float> feed_forward(const std::vector<float>& inputs) {
        Eigen::VectorXf y = Eigen::Map<const Eigen::VectorXf>(inputs.data(), inputs.size());

        for (size_t i = 0; i < weights.size(); ++i) {
            y.conservativeResize(config[i] - 1, 1); // Add bias
            y[config[i] - 1] = 1.0f;               // Set bias to 1
            y = weights[i] * y;

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
};

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

//void drawRectangle(float x, float y, float width, float height, bool filled, float r, float g, float b) {
//    glColor3f(r, g, b);
//    if (filled) {
//        glBegin(GL_QUADS);
//    } else {
//        glBegin(GL_LINE_LOOP);
//    }
//    glVertex2f(x, y);
//    glVertex2f(x + width, y);
//    glVertex2f(x + width, y + height);
//    glVertex2f(x, y + height);
//    glEnd();
//}

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
    // Draw outer rectangle
    //drawRectangle(-width * 0.5, -height * 0.5, width, height, false, 1.0f, 1.0f, 1.0f);

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

