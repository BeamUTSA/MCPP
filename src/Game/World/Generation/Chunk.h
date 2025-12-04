#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>
#include <queue>
#include <thread>

#include "Game/World/Generation/SurfaceManager.h"

namespace MCPP {
    class TextureAtlas;
    enum class BlockFace : uint8_t;
}

class MinecraftApp; // Forward declaration

struct ChunkVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec2 tileScale;
    float ao; // Ambient occlusion
};

class Chunk {
public:
    static constexpr int CHUNK_SIZE   = 16;
    static constexpr int CHUNK_HEIGHT = 128;

    // Chunk coordinates (not world-space position)
    glm::ivec3 chunkPos;

    explicit Chunk(const glm::ivec3& chunkPosition);
    ~Chunk();

    // Non-copyable, movable
    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;
    Chunk(Chunk&& other) noexcept;
    Chunk& operator=(Chunk&& other) noexcept;

    /**
     * Generate terrain for this chunk (fills m_blocks).
     * Mesh building is done separately via buildMesh().
     */
    void generate(const SurfaceManager& surfaceManager);

    /**
     * Build the mesh from block data.
     * @param app   Reference to MinecraftApp for world/block lookups across chunk borders.
     * @param atlas Reference to loaded texture atlas.
     */
    void buildMesh(const MinecraftApp& app, const MCPP::TextureAtlas& atlas);

    /**
     * Render the chunk.
     */
    void render() const;

    /**
     * Get block at local coordinates.
     */
    uint8_t getBlock(int x, int y, int z) const;

    /**
     * Set block at local coordinates.
     */
    void setBlock(int x, int y, int z, uint8_t type);

    /**
     * Check if chunk mesh needs rebuilding.
     */
    bool isDirty() const { return m_dirty; }

    /**
     * Mark chunk as needing mesh rebuild.
     */
    bool markDirty(bool dirty) { return m_dirty = dirty; }

    /**
     * Get world position of chunk origin.
     */
    glm::vec3 getWorldPosition() const {
        return glm::vec3(
            chunkPos.x * CHUNK_SIZE,
            chunkPos.y * CHUNK_HEIGHT,
            chunkPos.z * CHUNK_SIZE
        );
    }

    /**
     * World-space origin of this block (lower min corner of its unit cube).
     */
    glm::vec3 getBlockWorldPosition(int x, int y, int z) const {
        return glm::vec3(
            chunkPos.x * CHUNK_SIZE + x,
            chunkPos.y * CHUNK_HEIGHT + y,
            chunkPos.z * CHUNK_SIZE + z
        );
    }

    void uploadMesh(const std::vector<ChunkVertex>& vertices) {
        m_vertexCount = static_cast<uint32_t>(vertices.size());

        if (vertices.empty()) {
            m_dirty = false;
            return;
        }

        if (m_vao == 0) glGenVertexArrays(1, &m_vao);
        if (m_vbo == 0) glGenBuffers(1, &m_vbo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ChunkVertex), vertices.data(), GL_STATIC_DRAW);

        // === CORRECT ATTRIBUTE SETUP ===
        // 0: position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, position));
        glEnableVertexAttribArray(0);

        // 1: normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, normal));
        glEnableVertexAttribArray(1);

        // 2: texCoord
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, texCoord));
        glEnableVertexAttribArray(2);

        // 3: tileScale (vec2)
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, tileScale));
        glEnableVertexAttribArray(3);

        // 4: ao (float)
        glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, ao));
        glEnableVertexAttribArray(4);

        glBindVertexArray(0);
        m_dirty = false;
    }

    /**
     * Check if this chunk has any renderable geometry.
     */
    bool hasGeometry() const { return m_vertexCount > 0; }

private:
    // Block data storage: m_blocks[x][y][z]
    std::array<std::array<std::array<uint8_t, CHUNK_SIZE>, CHUNK_HEIGHT>, CHUNK_SIZE> m_blocks{};

    /*---------------------------------------------------------------------*/
    // OpenGL resources
    GLuint    m_vao{0};
    GLuint    m_vbo{0};
    GLuint    m_ebo{0};
    uint32_t  m_vertexCount{0};

    bool m_dirty{true};

    // Mesh building helpers
    void addFace(
        std::vector<ChunkVertex>& vertices,
        const glm::vec3& pos,
        MCPP::BlockFace face,
        const glm::vec2& uvMin,
        const glm::vec2& uvMax,
        const std::array<float, 4>& aoValues
    );

    bool  isBlockSolid (int x, int y, int z, const MinecraftApp& app) const;
    bool  isBlockOpaque(int x, int y, int z, const MinecraftApp& app) const;
    float calculateAO  (int x, int y, int z, int dx, int dy, int dz,
                        int axis, const MinecraftApp& app) const;
    
};
