#include "Game/World/Chunk.h"

#include <array>
#include <vector> // Added for dynamic mesh generation

namespace {
struct Vertex {
    glm::vec3 position;
    glm::vec3 color; // Added color attribute
    glm::vec3 normal;
};

// Define a single cube for now, will be replaced by dynamic mesh generation
// Using a green color for grass block
constexpr glm::vec3 GRASS_COLOR = {0.2f, 0.8f, 0.2f};

constexpr std::array<Vertex, 24> CUBE_VERTICES = {{
    // Front face
    {{-0.5f, -0.5f,  0.5f}, GRASS_COLOR, {0.0f, 0.0f, 1.0f}},
    {{ 0.5f, -0.5f,  0.5f}, GRASS_COLOR, {0.0f, 0.0f, 1.0f}},
    {{ 0.5f,  0.5f,  0.5f}, GRASS_COLOR, {0.0f, 0.0f, 1.0f}},
    {{-0.5f,  0.5f,  0.5f}, GRASS_COLOR, {0.0f, 0.0f, 1.0f}},
    // Back face
    {{-0.5f, -0.5f, -0.5f}, GRASS_COLOR, {0.0f, 0.0f, -1.0f}},
    {{ 0.5f, -0.5f, -0.5f}, GRASS_COLOR, {0.0f, 0.0f, -1.0f}},
    {{ 0.5f,  0.5f, -0.5f}, GRASS_COLOR, {0.0f, 0.0f, -1.0f}},
    {{-0.5f,  0.5f, -0.5f}, GRASS_COLOR, {0.0f, 0.0f, -1.0f}},
    // Left face
    {{-0.5f, -0.5f, -0.5f}, GRASS_COLOR, {-1.0f, 0.0f, 0.0f}},
    {{-0.5f, -0.5f,  0.5f}, GRASS_COLOR, {-1.0f, 0.0f, 0.0f}},
    {{-0.5f,  0.5f,  0.5f}, GRASS_COLOR, {-1.0f, 0.0f, 0.0f}},
    {{-0.5f,  0.5f, -0.5f}, GRASS_COLOR, {-1.0f, 0.0f, 0.0f}},
    // Right face
    {{ 0.5f, -0.5f, -0.5f}, GRASS_COLOR, {1.0f, 0.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, GRASS_COLOR, {1.0f, 0.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, GRASS_COLOR, {1.0f, 0.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, GRASS_COLOR, {1.0f, 0.0f, 0.0f}},
    // Top face
    {{-0.5f,  0.5f, -0.5f}, GRASS_COLOR, {0.0f, 1.0f, 0.0f}},
    {{-0.5f,  0.5f,  0.5f}, GRASS_COLOR, {0.0f, 1.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, GRASS_COLOR, {0.0f, 1.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, GRASS_COLOR, {0.0f, 1.0f, 0.0f}},
    // Bottom face
    {{-0.5f, -0.5f, -0.5f}, GRASS_COLOR, {0.0f, -1.0f, 0.0f}},
    {{-0.5f, -0.5f,  0.5f}, GRASS_COLOR, {0.0f, -1.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, GRASS_COLOR, {0.0f, -1.0f, 0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, GRASS_COLOR, {0.0f, -1.0f, 0.0f}},
}};

constexpr std::array<unsigned int, 36> CUBE_INDICES = {
    0, 1, 2, 2, 3, 0,       // Front
    5, 4, 7, 7, 6, 5,       // Back (corrected winding order)
    9, 8, 11, 11, 10, 9,    // Left (corrected winding order)
    12, 13, 14, 14, 15, 12, // Right
    17, 16, 19, 19, 18, 17, // Top (corrected winding order)
    20, 21, 22, 22, 23, 20  // Bottom
};
}

Chunk::Chunk(const glm::vec3& position) : position(position), VAO(0), VBO(0), EBO(0), m_indexCount(0) {
    // Initialize blocks array to 0 (air)
    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int y = 0; y < CHUNK_HEIGHT; ++y) {
            for (int z = 0; z < CHUNK_SIZE; ++z) {
                blocks[x][y][z] = 0;
            }
        }
    }
}

Chunk::~Chunk() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
}

void Chunk::generate() {
    // For now, generate a single block at y=0 for testing
    blocks[0][0][0] = 1; // Set block type to 1 (grass)

    buildMesh();
}

void Chunk::buildMesh() {
    std::vector<Vertex> meshVertices;
    std::vector<unsigned int> meshIndices;
    unsigned int indexOffset = 0;

    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int y = 0; y < CHUNK_HEIGHT; ++y) {
            for (int z = 0; z < CHUNK_SIZE; ++z) {
                if (blocks[x][y][z] == 0) continue; // Skip air blocks

                // Add a single cube for now, will be optimized later
                for (const auto& vertex : CUBE_VERTICES) {
                    meshVertices.push_back({
                        vertex.position + glm::vec3(x, y, z), // Offset position by block coordinates
                        vertex.color,
                        vertex.normal
                    });
                }
                for (unsigned int index : CUBE_INDICES) {
                    meshIndices.push_back(indexOffset + index);
                }
                indexOffset += CUBE_VERTICES.size();
            }
        }
    }

    if (VAO == 0) glGenVertexArrays(1, &VAO);
    if (VBO == 0) glGenBuffers(1, &VBO);
    if (EBO == 0) glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, meshVertices.size() * sizeof(Vertex), meshVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshIndices.size() * sizeof(unsigned int), meshIndices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);
    // Normal attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    m_indexCount = meshIndices.size(); // Set the actual number of indices
}


void Chunk::render() { // Removed const
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indexCount), GL_UNSIGNED_INT, 0); // Use m_indexCount
    glBindVertexArray(0);
}
