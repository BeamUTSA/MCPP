#include "Engine/Renderer/Shader.h"

#include <glad/glad.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector> // Not strictly needed here, but good practice for dynamic arrays

// Helper function to resolve include paths
std::string resolveIncludePath(const std::string& currentShaderPath, const std::string& includeFileName) {
    // Extract the directory from the current shader path
    size_t lastSlash = currentShaderPath.find_last_of("/\\");
    std::string shaderDir = "";
    if (lastSlash != std::string::npos) {
        shaderDir = currentShaderPath.substr(0, lastSlash + 1);
    }
    return shaderDir + includeFileName;
}

// Recursive function to load shader source and handle includes
std::string loadShaderSourceWithIncludes(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << path << std::endl;
        return ""; // Return empty string on failure
    }

    std::stringstream buffer;
    std::string line;
    while (std::getline(file, line)) {
        // Check for #include directive
        if (line.find("#include") != std::string::npos) {
            size_t start = line.find("\"") + 1;
            size_t end = line.find("\"", start);
            if (start != std::string::npos && end != std::string::npos) {
                std::string includeFileName = line.substr(start, end - start);
                std::string fullIncludePath = resolveIncludePath(path, includeFileName);
                buffer << loadShaderSourceWithIncludes(fullIncludePath); // Recursive call
            } else {
                std::cerr << "Warning: Malformed #include directive in " << path << ": " << line << std::endl;
                buffer << line << "\n"; // Include the malformed line anyway
            }
        } else {
            buffer << line << "\n";
        }
    }
    file.close();
    return buffer.str();
}


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
    } else {
        std::cout << "Shader program linked successfully!" << std::endl;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void Shader::use() const {
    glUseProgram(m_program);
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat) const {
    int location = glGetUniformLocation(m_program, name.c_str());
    if (location == -1) {
        std::cerr << "Warning: Uniform '" << name << "' not found in shader program." << std::endl;
    }
    glUniformMatrix4fv(location, 1, GL_FALSE, &mat[0][0]);
}

void Shader::setVec3(const std::string& name, const glm::vec3& vec) const {
    int location = glGetUniformLocation(m_program, name.c_str());
    if (location == -1) {
        std::cerr << "Warning: Uniform '" << name << "' not found in shader program." << std::endl;
    }
    glUniform3fv(location, 1, &vec[0]);
}

unsigned int Shader::loadShader(const std::string& path, unsigned int type) {
    std::string code = loadShaderSourceWithIncludes(path); // Use the new function
    if (code.empty()) {
        // Error already reported by loadShaderSourceWithIncludes
        return 0;
    }
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
    } else {
        std::cout << "Shader compiled successfully: " << path << std::endl;
    }

    return shader;
}
