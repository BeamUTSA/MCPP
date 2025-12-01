#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

class Chunk {
public:
    explicit Chunk(const glm::vec3& position);
    ~Chunk();

    void generate();
    void render() const;
    glm::vec3 getPosition() const { return position; }

private:
    glm::vec3 position;
    unsigned int vao{0};
    unsigned int vbo{0};
    unsigned int ebo{0};
};
