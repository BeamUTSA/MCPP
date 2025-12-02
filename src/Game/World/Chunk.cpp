#include "Game/World/Chunk.h"
#include "Game/World/Block/BlockDatabase.h"
#include "Game/Rendering/TextureAtlas.h"

#include <iostream>

// Face normals and vertex offsets for each face
namespace {

// Normals for each face
constexpr std::array<glm::vec3, 6> FACE_NORMALS = {{
    { 0.0f,  1.0f,  0.0f},  // Top
    { 0.0f, -1.0f,  0.0f},  // Bottom
    { 0.0f,  0.0f, -1.0f},  // North (-Z)
    { 0.0f,  0.0f,  1.0f},  // South (+Z)
    { 1.0f,  0.0f,  0.0f},  // East (+X)
    {-1.0f,  0.0f,  0.0f},  // West (-X)
}};

// Vertex positions for each face (4 corners, CCW winding)
// Each face has 4 vertices: bottom-left, bottom-right, top-right, top-left
constexpr std::array<std::array<glm::vec3, 4>, 6> FACE_VERTICES = {{
    // Top (Y+)
    {{ {0, 1, 0}, {1, 1, 0}, {1, 1, 1}, {0, 1, 1} }},
    // Bottom (Y-)
    {{ {0, 0, 1}, {1, 0, 1}, {1, 0, 0}, {0, 0, 0} }},
    // North (Z-)
    {{ {1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0} }},
    // South (Z+)
    {{ {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1} }},
    // East (X+)
    {{ {1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1} }},
    // West (X-)
    {{ {0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0} }},
}};

// UV coordinates for face corners (matches vertex order)
constexpr std::array<glm::vec2, 4> BASE_UVS = {{
    {0.0f, 0.0f},  // bottom-left
    {1.0f, 0.0f},  // bottom-right
    {1.0f, 1.0f},  // top-right
    {0.0f, 1.0f},  // top-left
}};

// Index pattern for a quad (two triangles, CCW)
constexpr std::array<uint32_t, 6> QUAD_INDICES = {0, 1, 2, 2, 3, 0};

} // anonymous namespace

Chunk::Chunk(const glm::ivec3& chunkPosition) : chunkPos(chunkPosition) {
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

        chunkPos = other.chunkPos;
        m_blocks = std::move(other.m_blocks);
        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        m_ebo = other.m_ebo;
        m_indexCount = other.m_indexCount;
        m_dirty = other.m_dirty;

        other.m_vao = 0;
        other.m_vbo = 0;
        other.m_ebo = 0;
        other.m_indexCount = 0;
    }
    return *this;
}

void Chunk::generate() {
    // Simple superflat generation for now
    // Layer 0: Bedrock
    // Layers 1-3: Stone
    // Layers 4-5: Dirt
    // Layer 6: Grass

    auto& db = MCPP::BlockDatabase::instance();

    const auto* bedrock = db.getBlockByName("Bedrock");
    const auto* stone = db.getBlockByName("Stone");
    const auto* dirt = db.getBlockByName("Dirt");
    const auto* grass = db.getBlockByName("Grass");

    uint8_t bedrockId = bedrock ? bedrock->id : 1;
    uint8_t stoneId = stone ? stone->id : 1;
    uint8_t dirtId = dirt ? dirt->id : 2;
    uint8_t grassId = grass ? grass->id : 3;

    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int z = 0; z < CHUNK_SIZE; ++z) {
            setBlock(x, 0, z, bedrockId);

            for (int y = 1; y <= 3; ++y) {
                setBlock(x, y, z, stoneId);
            }

            for (int y = 4; y <= 5; ++y) {
                setBlock(x, y, z, dirtId);
            }

            setBlock(x, 6, z, grassId);
        }
    }

    m_dirty = true;
}

uint8_t Chunk::getBlock(int x, int y, int z) const {
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE) {
        return 0; // Air for out of bounds
    }
    return m_blocks[x][y][z];
}

void Chunk::setBlock(int x, int y, int z, uint8_t type) {
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE) {
        return;
    }
    m_blocks[x][y][z] = type;
    m_dirty = true;
}

bool Chunk::isBlockSolid(int x, int y, int z) const {
    uint8_t block = getBlock(x, y, z);
    if (block == 0) return false;
    return MCPP::BlockDatabase::instance().isSolid(block);
}

bool Chunk::isBlockOpaque(int x, int y, int z) const {
    uint8_t block = getBlock(x, y, z);
    if (block == 0) return false;
    return MCPP::BlockDatabase::instance().isOpaque(block);
}

float Chunk::calculateAO(int x, int y, int z, int dx, int dy, int dz, int axis) const {
    // Simple ambient occlusion calculation
    // Check 3 neighbors around the vertex and darken based on how many are solid

    int side1 = 0, side2 = 0, corner = 0;

    // Determine which neighbors to check based on face orientation
    if (axis == 0) { // X axis (East/West faces)
        side1 = isBlockSolid(x + dx, y + dy, z) ? 1 : 0;
        side2 = isBlockSolid(x + dx, y, z + dz) ? 1 : 0;
        corner = isBlockSolid(x + dx, y + dy, z + dz) ? 1 : 0;
    } else if (axis == 1) { // Y axis (Top/Bottom faces)
        side1 = isBlockSolid(x + dx, y + dy, z) ? 1 : 0;
        side2 = isBlockSolid(x, y + dy, z + dz) ? 1 : 0;
        corner = isBlockSolid(x + dx, y + dy, z + dz) ? 1 : 0;
    } else { // Z axis (North/South faces)
        side1 = isBlockSolid(x + dx, y, z + dz) ? 1 : 0;
        side2 = isBlockSolid(x, y + dy, z + dz) ? 1 : 0;
        corner = isBlockSolid(x + dx, y + dy, z + dz) ? 1 : 0;
    }

    // AO formula: if both sides are solid, corner doesn't matter
    int ao;
    if (side1 && side2) {
        ao = 0;
    } else {
        ao = 3 - (side1 + side2 + corner);
    }

    // Convert to 0-1 range (0 = darkest, 1 = brightest)
    return 0.5f + 0.5f * (ao / 3.0f);
}

void Chunk::addFace(
    std::vector<ChunkVertex>& vertices,
    std::vector<uint32_t>& indices,
    const glm::vec3& pos,
    MCPP::BlockFace face,
    const glm::vec2& uvMin,
    const glm::vec2& uvMax,
    const std::array<float, 4>& aoValues
) {
    uint32_t baseIndex = static_cast<uint32_t>(vertices.size());
    int faceIdx = static_cast<int>(face);

    const auto& faceVerts = FACE_VERTICES[faceIdx];
    const auto& normal = FACE_NORMALS[faceIdx];

    // Calculate UV size
    glm::vec2 uvSize = uvMax - uvMin;

    // Add 4 vertices for this face
    for (int i = 0; i < 4; ++i) {
        ChunkVertex v;
        v.position = pos + faceVerts[i];
        v.normal = normal;

        // Map base UVs to atlas region
        v.texCoord = uvMin + BASE_UVS[i] * uvSize;

        v.ao = aoValues[i];

        vertices.push_back(v);
    }

    // Add 6 indices for 2 triangles
    for (uint32_t idx : QUAD_INDICES) {
        indices.push_back(baseIndex + idx);
    }
}

void Chunk::buildMesh(const MCPP::TextureAtlas& atlas) {
    std::vector<ChunkVertex> vertices;
    std::vector<uint32_t> indices;

    // Reserve approximate space
    vertices.reserve(CHUNK_SIZE * CHUNK_SIZE * 64);
    indices.reserve(CHUNK_SIZE * CHUNK_SIZE * 96);

    auto& db = MCPP::BlockDatabase::instance();

    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int y = 0; y < CHUNK_HEIGHT; ++y) {
            for (int z = 0; z < CHUNK_SIZE; ++z) {
                uint8_t block = m_blocks[x][y][z];
                if (block == 0) continue; // Skip air

                const auto& blockDef = db.getBlock(block);
                glm::vec3 pos(x, y, z);

                // Check each face
                // Top (Y+)
                if (!isBlockOpaque(x, y + 1, z)) {
                    auto uv = db.getBlockFaceUV(block, MCPP::BlockFace::Top);
                    std::array<float, 4> ao = {1.0f, 1.0f, 1.0f, 1.0f}; // Simplified AO for now
                    addFace(vertices, indices, pos, MCPP::BlockFace::Top, uv.min, uv.max, ao);
                }

                // Bottom (Y-)
                if (!isBlockOpaque(x, y - 1, z)) {
                    auto uv = db.getBlockFaceUV(block, MCPP::BlockFace::Bottom);
                    std::array<float, 4> ao = {0.8f, 0.8f, 0.8f, 0.8f}; // Bottom is darker
                    addFace(vertices, indices, pos, MCPP::BlockFace::Bottom, uv.min, uv.max, ao);
                }

                // North (Z-)
                if (!isBlockOpaque(x, y, z - 1)) {
                    auto uv = db.getBlockFaceUV(block, MCPP::BlockFace::North);
                    std::array<float, 4> ao = {0.9f, 0.9f, 0.9f, 0.9f};
                    addFace(vertices, indices, pos, MCPP::BlockFace::North, uv.min, uv.max, ao);
                }

                // South (Z+)
                if (!isBlockOpaque(x, y, z + 1)) {
                    auto uv = db.getBlockFaceUV(block, MCPP::BlockFace::South);
                    std::array<float, 4> ao = {0.9f, 0.9f, 0.9f, 0.9f};
                    addFace(vertices, indices, pos, MCPP::BlockFace::South, uv.min, uv.max, ao);
                }

                // East (X+)
                if (!isBlockOpaque(x + 1, y, z)) {
                    auto uv = db.getBlockFaceUV(block, MCPP::BlockFace::East);
                    std::array<float, 4> ao = {0.85f, 0.85f, 0.85f, 0.85f};
                    addFace(vertices, indices, pos, MCPP::BlockFace::East, uv.min, uv.max, ao);
                }

                // West (X-)
                if (!isBlockOpaque(x - 1, y, z)) {
                    auto uv = db.getBlockFaceUV(block, MCPP::BlockFace::West);
                    std::array<float, 4> ao = {0.85f, 0.85f, 0.85f, 0.85f};
                    addFace(vertices, indices, pos, MCPP::BlockFace::West, uv.min, uv.max, ao);
                }
            }
        }
    }

    m_indexCount = static_cast<uint32_t>(indices.size());

    if (m_indexCount == 0) {
        m_dirty = false;
        return;
    }

    // Create/update OpenGL buffers
    if (m_vao == 0) glGenVertexArrays(1, &m_vao);
    if (m_vbo == 0) glGenBuffers(1, &m_vbo);
    if (m_ebo == 0) glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ChunkVertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    // Position (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, position));
    glEnableVertexAttribArray(0);

    // Normal (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, normal));
    glEnableVertexAttribArray(1);

    // TexCoord (location 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, texCoord));
    glEnableVertexAttribArray(2);

    // AO (location 3)
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, ao));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    m_dirty = false;

    std::cout << "Chunk built: " << vertices.size() << " vertices, " << m_indexCount << " indices" << std::endl;
}

void Chunk::render() const {
    if (m_indexCount == 0 || m_vao == 0) return;

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indexCount), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}