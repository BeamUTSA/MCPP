#include "Engine/Renderer/Shader.h"

#include <glad/glad.h>
#include <fstream>
#include <iostream>
#include <sstream>

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
    unsigned int vertex = loadShader(vertexPath, GL_VERTEX_SHADER);
    unsigned int fragment = loadShader(fragmentPath, GL_FRAGMENT_SHADER);

    m_program = glCreateProgram();
    glAttachShader(m_program, vertex);
    glAttachShader(m_program, fragment);
    glLinkProgram(m_program);

    int success;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_program, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void Shader::use() const {
    glUseProgram(m_program);
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat) const {
    int location = glGetUniformLocation(m_program, name.c_str());
    glUniformMatrix4fv(location, 1, GL_FALSE, &mat[0][0]);
}

void Shader::setVec3(const std::string& name, const glm::vec3& vec) const {
    int location = glGetUniformLocation(m_program, name.c_str());
    glUniform3fv(location, 1, &vec[0]);
}

unsigned int Shader::loadShader(const std::string& path, unsigned int type) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string code = buffer.str();
    const char* source = code.c_str();

    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed (" << path << "): " << infoLog << std::endl;
    }

    return shader;
}
