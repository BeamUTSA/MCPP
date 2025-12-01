#pragma once

#include <glm/glm.hpp>
#include <string>

class Shader {
public:
    Shader() = default;
    Shader(const std::string& vertexPath, const std::string& fragmentPath);

    void use() const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;
    void setVec3(const std::string& name, const glm::vec3& vec) const;

private:
    unsigned int loadShader(const std::string& path, unsigned int type);

    unsigned int m_program{0};
};
