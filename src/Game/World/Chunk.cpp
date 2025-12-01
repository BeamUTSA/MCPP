#include "Game/World/Chunk.h"

#include <array>

namespace {
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

constexpr std::array<Vertex, 24> CUBE_VERTICES = {{
    // Front
    {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
    {{0.5f, -0.5f,  0.5f},  {0.0f, 0.0f, 1.0f}},
    {{0.5f,  0.5f,  0.5f},  {0.0f, 0.0f, 1.0f}},
    {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
    // Back
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}},
    {{0.5f, -0.5f, -0.5f},  {0.0f, 0.0f, -1.0f}},
    {{0.5f,  0.5f, -0.5f},  {0.0f, 0.0f, -1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}},
    // Left
    {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}},
    {{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}},
    {{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}},
    {{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}},
    // Right
    {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    // Top
    {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f,  0.5f},  {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f},  {0.0f, 1.0f, 0.0f}},
    // Bottom
    {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}},
    {{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}},
    {{0.5f, -0.5f,  0.5f},  {0.0f, -1.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f},  {0.0f, -1.0f, 0.0f}},
}};

constexpr std::array<unsigned int, 36> CUBE_INDICES = {
    0, 1, 2, 2, 3, 0,       // Front
    4, 5, 6, 6, 7, 4,       // Back
    8, 9, 10, 10, 11, 8,    // Left
    12, 13, 14, 14, 15, 12, // Right
    16, 17, 18, 18, 19, 16, // Top
    20, 21, 22, 22, 23, 20  // Bottom
};
}

Chunk::Chunk(const glm::vec3& position) : position(position) {}

Chunk::~Chunk() {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (ebo) glDeleteBuffers(1, &ebo);
}

void Chunk::generate() {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VERTICES), CUBE_VERTICES.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(CUBE_INDICES), CUBE_INDICES.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Chunk::render() const {
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(CUBE_INDICES.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
