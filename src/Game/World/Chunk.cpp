#include "Chunk.h"
#include <cstring>

Chunk::Chunk(glm::vec3 position) : position(position) {
    memset(blocks, 0, sizeof(blocks));
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
}

Chunk::~Chunk() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void Chunk::generate() {
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            blocks[x][0][z] = 1; // Grass
        }
    }
    buildMesh();
}

void Chunk::buildMesh() {
    vertices.clear();
    
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                if (blocks[x][y][z] == 0) continue;
                
                glm::vec3 blockPos(x, y, z);
                
                if (!isBlockSolid(x, y + 1, z)) addFace(blockPos, 0); // Top
                if (!isBlockSolid(x, y - 1, z)) addFace(blockPos, 1); // Bottom
                if (!isBlockSolid(x, y, z + 1)) addFace(blockPos, 2); // Front
                if (!isBlockSolid(x, y, z - 1)) addFace(blockPos, 3); // Back
                if (!isBlockSolid(x + 1, y, z)) addFace(blockPos, 4); // Right
                if (!isBlockSolid(x - 1, y, z)) addFace(blockPos, 5); // Left
            }
        }
    }
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Chunk::addFace(glm::vec3 pos, int face) {
    float x = pos.x;
    float y = pos.y;
    float z = pos.z;
    
    float r = 0.2f, g = 0.8f, b = 0.2f;
    
    glm::vec3 v0 = {x, y, z};
    glm::vec3 v1 = {x+1, y, z};
    glm::vec3 v2 = {x+1, y+1, z};
    glm::vec3 v3 = {x, y+1, z};
    glm::vec3 v4 = {x, y, z+1};
    glm::vec3 v5 = {x+1, y, z+1};
    glm::vec3 v6 = {x+1, y+1, z+1};
    glm::vec3 v7 = {x, y+1, z+1};

    float faceVertices[6][6][6] = {
        {{v3.x, v3.y, v3.z, r, g, b}, {v2.x, v2.y, v2.z, r, g, b}, {v6.x, v6.y, v6.z, r, g, b},
         {v3.x, v3.y, v3.z, r, g, b}, {v6.x, v6.y, v6.z, r, g, b}, {v7.x, v7.y, v7.z, r, g, b}},
        
        {{v0.x, v0.y, v0.z, r*0.5f, g*0.5f, b*0.5f}, {v4.x, v4.y, v4.z, r*0.5f, g*0.5f, b*0.5f}, {v5.x, v5.y, v5.z, r*0.5f, g*0.5f, b*0.5f},
         {v0.x, v0.y, v0.z, r*0.5f, g*0.5f, b*0.5f}, {v5.x, v5.y, v5.z, r*0.5f, g*0.5f, b*0.5f}, {v1.x, v1.y, v1.z, r*0.5f, g*0.5f, b*0.5f}},
        
        {{v4.x, v4.y, v4.z, r*0.8f, g*0.8f, b*0.8f}, {v7.x, v7.y, v7.z, r*0.8f, g*0.8f, b*0.8f}, {v6.x, v6.y, v6.z, r*0.8f, g*0.8f, b*0.8f},
         {v4.x, v4.y, v4.z, r*0.8f, g*0.8f, b*0.8f}, {v6.x, v6.y, v6.z, r*0.8f, g*0.8f, b*0.8f}, {v5.x, v5.y, v5.z, r*0.8f, g*0.8f, b*0.8f}},
        
        {{v0.x, v0.y, v0.z, r*0.8f, g*0.8f, b*0.8f}, {v1.x, v1.y, v1.z, r*0.8f, g*0.8f, b*0.8f}, {v2.x, v2.y, v2.z, r*0.8f, g*0.8f, b*0.8f},
         {v0.x, v0.y, v0.z, r*0.8f, g*0.8f, b*0.8f}, {v2.x, v2.y, v2.z, r*0.8f, g*0.8f, b*0.8f}, {v3.x, v3.y, v3.z, r*0.8f, g*0.8f, b*0.8f}},
        
        {{v1.x, v1.y, v1.z, r*0.6f, g*0.6f, b*0.6f}, {v5.x, v5.y, v5.z, r*0.6f, g*0.6f, b*0.6f}, {v6.x, v6.y, v6.z, r*0.6f, g*0.6f, b*0.6f},
         {v1.x, v1.y, v1.z, r*0.6f, g*0.6f, b*0.6f}, {v6.x, v6.y, v6.z, r*0.6f, g*0.6f, b*0.6f}, {v2.x, v2.y, v2.z, r*0.6f, g*0.6f, b*0.6f}},
        
        {{v0.x, v0.y, v0.z, r*0.6f, g*0.6f, b*0.6f}, {v3.x, v3.y, v3.z, r*0.6f, g*0.6f, b*0.6f}, {v7.x, v7.y, v7.z, r*0.6f, g*0.6f, b*0.6f},
         {v0.x, v0.y, v0.z, r*0.6f, g*0.6f, b*0.6f}, {v7.x, v7.y, v7.z, r*0.6f, g*0.6f, b*0.6f}, {v4.x, v4.y, v4.z, r*0.6f, g*0.6f, b*0.6f}}
    };
    
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 6; j++) {
            vertices.push_back(faceVertices[face][i][j]);
        }
    }
}

bool Chunk::isBlockSolid(int x, int y, int z) {
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE)
        return false;
    return blocks[x][y][z] != 0;
}

void Chunk::render() {
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 6);
    glBindVertexArray(0);
}

unsigned char Chunk::getBlock(int x, int y, int z) {
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE)
        return 0;
    return blocks[x][y][z];
}

void Chunk::setBlock(int x, int y, int z, unsigned char type) {
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE)
        return;
    blocks[x][y][z] = type;
}