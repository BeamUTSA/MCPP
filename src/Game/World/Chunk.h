#ifndef CHUNK_H
#define CHUNK_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class Chunk {
public:
    static const int CHUNK_SIZE = 16;
    static const int CHUNK_HEIGHT = 16;
    
    glm::vec3 position; // Made public

    Chunk(const glm::vec3& position); // Updated constructor signature
    ~Chunk();
    
    void generate();
    void render();
    
    unsigned char getBlock(int x, int y, int z);
    void setBlock(int x, int y, int z, unsigned char type);
    
private:
    unsigned char blocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE];
    
    unsigned int VAO, VBO, EBO; // Added EBO
    unsigned int m_indexCount; // Added m_indexCount
    std::vector<float> vertices;
    
    void buildMesh();
    void addFace(glm::vec3 pos, int face);
    bool isBlockSolid(int x, int y, int z);
};

#endif