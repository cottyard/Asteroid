#include <iostream>
#include <fstream>
#include <cmath>
#include "GL/glew.h"

GLuint glowShader;

char* readFileAsCharArray(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Could not open the file " << filename << std::endl;
        return nullptr;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    char* buffer = new char[size + 1];
    if (file.read(buffer, size)) {
        buffer[size] = '\0';
        return buffer;
    } else {
        delete[] buffer;
        std::cerr << "Error reading file " << filename << std::endl;
        return nullptr;
    }
}

void setupShaders() {
	char* vertexShaderSource = readFileAsCharArray("shaders/vertex.txt");
	char* fragmentShaderSource = readFileAsCharArray("shaders/fragment.txt");

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    glowShader = glCreateProgram();
    glAttachShader(glowShader, vertexShader);
    glAttachShader(glowShader, fragmentShader);
    glLinkProgram(glowShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void useShader_Glow(){
    glUseProgram(glowShader);
    glUniform2f(glGetUniformLocation(glowShader, "lineStart"), 0.3f, 0.5f);
    glUniform2f(glGetUniformLocation(glowShader, "lineEnd"), 0.7f, 0.5f);
    glUniform3f(glGetUniformLocation(glowShader, "glowColor"), 1.0f, 0.5f, 0.0f);
    glUniform1f(glGetUniformLocation(glowShader, "glowIntensity"), 0.01f);
}

void endShader(){
	glUseProgram(0);
}

void initShaders(){	
	glewInit();
	setupShaders();
}

