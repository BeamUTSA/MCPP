#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>

namespace MCPP {
    class TextureAtlas;
    enum class BlockFace : uint8_t;
}

struct ChunkVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    float ao; // Ambient occlusion
};

class Chunk {
public:
    static constexpr int CHUNK_SIZE = 16;
    static constexpr int CHUNK_HEIGHT = 256;

    glm::ivec3 chunkPos; // Chunk coordinates (not world position)

    explicit Chunk(const glm::ivec3& chunkPosition);
    ~Chunk();

    // Non-copyable, movable
    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;
    Chunk(Chunk&& other) noexcept;
    Chunk& operator=(Chunk&& other) noexcept;

    /**
     * Generate terrain for this chunk
     */
    void generate();

    /**
     * Build the mesh from block data
     * @param atlas Texture atlas for UV lookups
     */
    void buildMesh(const MCPP::TextureAtlas& atlas);

    /**
     * Render the chunk
     */
    void render() const;

    /**
     * Get block at local coordinates
     */
    uint8_t getBlock(int x, int y, int z) const;

    /**
     * Set block at local coordinates
     */
    void setBlock(int x, int y, int z, uint8_t type);

    /**
     * Check if chunk mesh needs rebuilding
     */
    bool isDirty() const { return m_dirty; }

    /**
     * Mark chunk as needing mesh rebuild
     */
    void markDirty() { m_dirty = true; }

    /**
     * Get world position of chunk origin
     */
    glm::vec3 getWorldPosition() const {
        return glm::vec3(
            chunkPos.x * CHUNK_SIZE,
            chunkPos.y * CHUNK_HEIGHT,
            chunkPos.z * CHUNK_SIZE
        );
    }

    /**
     * Check if this chunk has any renderable geometry
     */
    bool hasGeometry() const { return m_indexCount > 0; }

private:
    // Block data storage (y is height, inner-most for cache efficiency during generation)
    std::array<std::array<std::array<uint8_t, CHUNK_SIZE>, CHUNK_HEIGHT>, CHUNK_SIZE> m_blocks{};

    // OpenGL resources
    GLuint m_vao{0};
    GLuint m_vbo{0};
    GLuint m_ebo{0};
    uint32_t m_indexCount{0};

    bool m_dirty{true};

    // Mesh building helpers
    void addFace(
        std::vector<ChunkVertex>& vertices,
        std::vector<uint32_t>& indices,
        const glm::vec3& pos,
        MCPP::BlockFace face,
        const glm::vec2& uvMin,
        const glm::vec2& uvMax,
        const std::array<float, 4>& aoValues
    );

    bool isBlockSolid(int x, int y, int z) const;
    bool isBlockOpaque(int x, int y, int z) const;
    float calculateAO(int x, int y, int z, int dx, int dy, int dz, int axis) const;
};