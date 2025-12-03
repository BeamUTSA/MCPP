#include "Chunk.h"
#include "Game/World/Block/BlockDatabase.h"
#include "Game/Rendering/TextureAtlas.h"
#include "Game/MinecraftApp.h"
#include "Game/World/Generation/SurfaceManager.h"
#include "Game/World/Meshing/Greedy.h"

#include <iostream>

namespace {

// Normals for each face (matches BlockFace order)
constexpr std::array<glm::vec3, 6> FACE_NORMALS = {{
    { 0.0f,  1.0f,  0.0f},  // Top (Y+)
    { 0.0f, -1.0f,  0.0f},  // Bottom (Y-)
    { 0.0f,  0.0f, -1.0f},  // North (Z-)
    { 0.0f,  0.0f,  1.0f},  // South (Z+)
    { 1.0f,  0.0f,  0.0f},  // East  (X+)
    {-1.0f,  0.0f,  0.0f},  // West  (X-)
}};

// We use ONE consistent pattern for all faces:
// v0 = bottom-left, v1 = bottom-right, v2 = top-right, v3 = top-left
// Triangles: (v0,v1,v2) and (v0,v2,v3)
constexpr std::array<std::array<glm::vec3, 6>, 6> FACE_VERTEX_POSITIONS = {{

    // Top (Y+), looking down: +Z forward, +X right
    // v0=(0,1,0), v1=(1,1,0), v2=(1,1,1), v3=(0,1,1)
    {{
        {0,1,0}, {1,1,0}, {1,1,1},
        {0,1,0}, {1,1,1}, {0,1,1}
    }},

    // Bottom (Y-), looking up
    // v0=(0,0,1), v1=(1,0,1), v2=(1,0,0), v3=(0,0,0)
    {{
        {0,0,1}, {1,0,1}, {1,0,0},
        {0,0,1}, {1,0,0}, {0,0,0}
    }},

    // North (Z-), looking from +Z toward -Z
    // v0=(1,0,0), v1=(0,0,0), v2=(0,1,0), v3=(1,1,0)
    {{
        {1,0,0}, {0,0,0}, {0,1,0},
        {1,0,0}, {0,1,0}, {1,1,0}
    }},

    // South (Z+), looking from -Z toward +Z
    // v0=(0,0,1), v1=(1,0,1), v2=(1,1,1), v3=(0,1,1)
    {{
        {0,0,1}, {1,0,1}, {1,1,1},
        {0,0,1}, {1,1,1}, {0,1,1}
    }},

    // East (X+), looking from -X toward +X
    // v0=(1,0,0), v1=(1,0,1), v2=(1,1,1), v3=(1,1,0)
    {{
        {1,0,0}, {1,0,1}, {1,1,1},
        {1,0,0}, {1,1,1}, {1,1,0}
    }},

    // West (X-), looking from +X toward -X
    // v0=(0,0,1), v1=(0,0,0), v2=(0,1,0), v3=(0,1,1)
    {{
        {0,0,1}, {0,0,0}, {0,1,0},
        {0,0,1}, {0,1,0}, {0,1,1}
    }},
}};

// All faces share the same UV pattern (matches the v0..v3 order above)
constexpr std::array<std::array<glm::vec2, 6>, 6> FACE_UVS = {{
    {{
        {0,0}, {1,0}, {1,1},
        {0,0}, {1,1}, {0,1}
    }},
    {{
        {0,0}, {1,0}, {1,1},
        {0,0}, {1,1}, {0,1}
    }},
    {{
        {0,0}, {1,0}, {1,1},
        {0,0}, {1,1}, {0,1}
    }},
    {{
        {0,0}, {1,0}, {1,1},
        {0,0}, {1,1}, {0,1}
    }},
    {{
        {0,0}, {1,0}, {1,1},
        {0,0}, {1,1}, {0,1}
    }},
    {{
        {0,0}, {1,0}, {1,1},
        {0,0}, {1,1}, {0,1}
    }},
}};

} // anonymous namespace

Chunk::Chunk(const glm::ivec3& chunkPosition)
    : chunkPos(chunkPosition) {
    // Initialize all blocks to air
    for (auto& xSlice : m_blocks) {
        for (auto& ySlice : xSlice) {
            ySlice.fill(0);
        }
    }
}

Chunk::~Chunk() {
    if (m_vao != 0) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
    if (m_ebo != 0) glDeleteBuffers(1, &m_ebo);
}

Chunk::Chunk(Chunk&& other) noexcept
    : chunkPos(other.chunkPos)
    , m_blocks(std::move(other.m_blocks))
    , m_vao(other.m_vao)
    , m_vbo(other.m_vbo)
    , m_ebo(other.m_ebo)
    , m_indexCount(other.m_indexCount)
    , m_dirty(other.m_dirty) {
    other.m_vao = 0;
    other.m_vbo = 0;
    other.m_ebo = 0;
    other.m_indexCount = 0;
}

Chunk& Chunk::operator=(Chunk&& other) noexcept {
    if (this != &other) {
        if (m_vao != 0) glDeleteVertexArrays(1, &m_vao);
        if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
        if (m_ebo != 0) glDeleteBuffers(1, &m_ebo);

        chunkPos      = other.chunkPos;
        m_blocks      = std::move(other.m_blocks);
        m_vao         = other.m_vao;
        m_vbo         = other.m_vbo;
        m_ebo         = other.m_ebo;
        m_indexCount  = other.m_indexCount;
        m_dirty       = other.m_dirty;

        other.m_vao        = 0;
        other.m_vbo        = 0;
        other.m_ebo        = 0;
        other.m_indexCount = 0;
    }
    return *this;
}

void Chunk::generate(const SurfaceManager& surfaceManager) {
    auto& db = MCPP::BlockDatabase::instance();

    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int z = 0; z < CHUNK_SIZE; ++z) {
            const int worldX = chunkPos.x * CHUNK_SIZE + x;
            const int worldZ = chunkPos.z * CHUNK_SIZE + z;

            SurfaceSample s = surfaceManager.sampleColumn(worldX, worldZ);

            // Clamp height into chunk vertical range
            int maxY = std::min(s.height, CHUNK_HEIGHT - 1);

            for (int y = 0; y < CHUNK_HEIGHT; ++y) {
                uint8_t blockId = 0; // air

                if (y > maxY) {
                    blockId = 0; // air
                } else if (y == s.height) {
                    blockId = s.topBlock;
                } else if (y >= s.height - 3) {
                    blockId = s.fillerBlock;
                } else {
                    blockId = s.stoneBlock;
                }

                setBlock(x, y, z, blockId);
            }
        }
    }

    // NOTE: Don't build the mesh here. ChunkManager will call buildMesh()
    // with the correct MinecraftApp & TextureAtlas instances.
    m_dirty = true;
}

uint8_t Chunk::getBlock(int x, int y, int z) const {
    if (x < 0 || x >= CHUNK_SIZE ||
        y < 0 || y >= CHUNK_HEIGHT ||
        z < 0 || z >= CHUNK_SIZE) {
        return 0; // air
    }
    return m_blocks[x][y][z];
}

void Chunk::setBlock(int x, int y, int z, uint8_t type) {
    if (x < 0 || x >= CHUNK_SIZE ||
        y < 0 || y >= CHUNK_HEIGHT ||
        z < 0 || z >= CHUNK_SIZE) {
        return;
    }
    m_blocks[x][y][z] = type;
    m_dirty = true;
}

bool Chunk::isBlockSolid(int x, int y, int z, const MinecraftApp& app) const {
    // First, check local storage
    if (x >= 0 && x < CHUNK_SIZE &&
        y >= 0 && y < CHUNK_HEIGHT &&
        z >= 0 && z < CHUNK_SIZE) {
        uint8_t block = m_blocks[x][y][z];
        if (block == 0) return false;
        return MCPP::BlockDatabase::instance().isSolid(block);
    }

    // Only fall back to world lookup when crossing chunk boundaries
    glm::ivec3 worldPos = glm::ivec3(getBlockWorldPosition(x, y, z));
    uint8_t    worldBlk = app.getBlock(worldPos);
    if (worldBlk == 0) return false;
    return MCPP::BlockDatabase::instance().isSolid(worldBlk);
}

bool Chunk::isBlockOpaque(int x, int y, int z, const MinecraftApp& app) const {
    if (x >= 0 && x < CHUNK_SIZE &&
        y >= 0 && y < CHUNK_HEIGHT &&
        z >= 0 && z < CHUNK_SIZE) {
        uint8_t block = m_blocks[x][y][z];
        if (block == 0) return false;
        return MCPP::BlockDatabase::instance().isOpaque(block);
    }

    glm::ivec3 worldPos = glm::ivec3(getBlockWorldPosition(x, y, z));
    uint8_t    block    = app.getBlock(worldPos);
    if (block == 0) return false;
    return MCPP::BlockDatabase::instance().isOpaque(block);
}

float Chunk::calculateAO(int x, int y, int z,
                         int dx, int dy, int dz,
                         int axis, const MinecraftApp& app) const {
    // Currently unused in buildMesh (we pass constant AO),
    // but left here for future nice-looking AO.
    int side1 = 0, side2 = 0, corner = 0;

    if (axis == 0) {          // X faces
        side1  = isBlockSolid(x + dx, y + dy, z,       app) ? 1 : 0;
        side2  = isBlockSolid(x + dx, y,      z + dz,  app) ? 1 : 0;
        corner = isBlockSolid(x + dx, y + dy, z + dz,  app) ? 1 : 0;
    } else if (axis == 1) {   // Y faces
        side1  = isBlockSolid(x + dx, y + dy, z,       app) ? 1 : 0;
        side2  = isBlockSolid(x,      y + dy, z + dz,  app) ? 1 : 0;
        corner = isBlockSolid(x + dx, y + dy, z + dz,  app) ? 1 : 0;
    } else {                  // Z faces
        side1  = isBlockSolid(x + dx, y,      z + dz,  app) ? 1 : 0;
        side2  = isBlockSolid(x,      y + dy, z + dz,  app) ? 1 : 0;
        corner = isBlockSolid(x + dx, y + dy, z + dz,  app) ? 1 : 0;
    }

    int ao;
    if (side1 && side2) {
        ao = 0;
    } else {
        ao = 3 - (side1 + side2 + corner);
    }

    return 0.5f + 0.5f * (ao / 3.0f);
}

void Chunk::addFace(std::vector<ChunkVertex>& vertices,
                    const glm::vec3& pos,
                    MCPP::BlockFace face,
                    const glm::vec2& uvMin,
                    const glm::vec2& uvMax,
                    const std::array<float, 4>& aoValues) {

    const int faceIdx = static_cast<int>(face);
    const auto& normal        = FACE_NORMALS[faceIdx];
    const auto& facePositions = FACE_VERTEX_POSITIONS[faceIdx];
    const auto& faceUVs       = FACE_UVS[faceIdx];

    glm::vec2 uvSize = uvMax - uvMin;

    for (int i = 0; i < 6; ++i) {
        ChunkVertex v;
        v.position = pos + facePositions[i];
        v.normal   = normal;
        v.texCoord = uvMin + faceUVs[i] * uvSize;

        // Map AO per v0..v3, reused across the 2 triangles
        switch (i) {
            case 0: // v0
            case 3: // v0 (second triangle)
                v.ao = aoValues[0];
                break;
            case 1: // v1
                v.ao = aoValues[1];
                break;
            case 2: // v2
            case 4: // v2
                v.ao = aoValues[2];
                break;
            case 5: // v3
                v.ao = aoValues[3];
                break;
        }

        vertices.push_back(v);
    }
}

void Chunk::buildMesh(const MinecraftApp& app, const MCPP::TextureAtlas& atlas) {
    std::vector<ChunkVertex> vertices;

    // Build greedy mesh (merges faces before we touch any GL)
    Meshing::buildGreedyMesh(*this, app, vertices);

    m_indexCount = static_cast<uint32_t>(vertices.size());

    if (m_indexCount == 0) {
        m_dirty = false;
        return;
    }

    // --- The OpenGL buffer code you already have stays the same ---
    if (m_vao == 0) glGenVertexArrays(1, &m_vao);
    if (m_vbo == 0) glGenBuffers(1, &m_vbo);
    if (m_ebo != 0) {
        glDeleteBuffers(1, &m_ebo);
        m_ebo = 0;
    }

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(ChunkVertex),
                 vertices.data(),
                 GL_STATIC_DRAW);

    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          sizeof(ChunkVertex),
                          (void*)offsetof(ChunkVertex, position));
    glEnableVertexAttribArray(0);

    // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          sizeof(ChunkVertex),
                          (void*)offsetof(ChunkVertex, normal));
    glEnableVertexAttribArray(1);

    // TexCoord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                          sizeof(ChunkVertex),
                          (void*)offsetof(ChunkVertex, texCoord));
    glEnableVertexAttribArray(2);

    // AO
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE,
                          sizeof(ChunkVertex),
                          (void*)offsetof(ChunkVertex, ao));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    m_dirty = false;
}

void Chunk::render() const {
    if (m_indexCount == 0 || m_vao == 0) return;

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_indexCount));
    glBindVertexArray(0);
}
