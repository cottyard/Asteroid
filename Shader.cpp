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

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cout << "Shader Compilation Error: " << infoLog << std::endl;
    }
    return shader;
}

void setupShaders() {
	char* vertexSource = readFileAsCharArray("shaders/vertex.txt");
	char* fragmentSource = readFileAsCharArray("shaders/fragment.txt");
	
    GLuint vertexShader =  compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    glowShader = glCreateProgram();
    glAttachShader(glowShader, vertexShader);
    glAttachShader(glowShader, fragmentShader);
    glLinkProgram(glowShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

