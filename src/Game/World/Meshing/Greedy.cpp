// Greedy.cpp - ULTRA OPTIMIZED 2025 Edition
#include "Game/World/Meshing/Greedy.h"
#include "Game/World/Generation/Chunk.h"
#include "Game/MinecraftApp.h"
#include "Game/World/Block/BlockDatabase.h"

#include <array>
#include <cstdint>

using MCPP::BlockDatabase;
using MCPP::BlockFace;

namespace Meshing {

// Precomputed face data (never changes)
struct FaceData {
    std::array<glm::vec3, 6> vertices;
    glm::vec3 normal;
};

static constexpr std::array<FaceData, 6> FACE_DATA{{
    // Top
    {{{{1,1,1},{1,1,0},{0,1,0},{0,1,0},{0,1,1},{1,1,1}}}, {0,1,0}},
    // Bottom
    {{{{0,0,0},{1,0,0},{1,0,1},{1,0,1},{0,0,1},{0,0,0}}}, {0,-1,0}},
    // North
    {{{{1,0,0},{0,0,0},{0,1,0},{0,1,0},{1,1,0},{1,0,0}}}, {0,0,-1}},
    // South
    {{{{1,1,1},{0,1,1},{0,0,1},{0,0,1},{1,0,1},{1,1,1}}}, {0,0,1}},
    // East
    {{{{1,1,1},{1,0,1},{1,0,0},{1,0,0},{1,1,0},{1,1,1}}}, {1,0,0}},
    // West
    {{{{0,1,1},{0,1,0},{0,0,0},{0,0,0},{0,0,1},{0,1,1}}}, {-1,0,0}},
}};

// Fast inline mask access
struct Mask {
    uint8_t id{0};
    bool visible{false};
};

static inline Mask& maskAt(Mask* mask, int stride, int x, int y) {
    return mask[y * stride + x];
}

// Ultra-fast greedy scan (no heap, no std::function)
using EmitFunc = void(*)(std::vector<ChunkVertex>&, int, int, int, int, int, uint8_t, BlockFace);

static void greedyScan2D(
    Mask* mask,
    int width,
    int height,
    EmitFunc emit,
    BlockFace face,
    int fixedCoord,
    std::vector<ChunkVertex>& out
) {
    for (int y = 0; y < height; ++y) {
        int x = 0;
        while (x < width) {
            Mask& cell = maskAt(mask, width, x, y);
            if (!cell.visible) { ++x; continue; }

            uint8_t id = cell.id;
            int quadW = 1;
            while (x + quadW < width && maskAt(mask, width, x + quadW, y).visible &&
                   maskAt(mask, width, x + quadW, y).id == id) ++quadW;

            int quadH = 1;
            bool canExpand = true;
            while (y + quadH < height && canExpand) {
                for (int w = 0; w < quadW; ++w) {
                    Mask& lower = maskAt(mask, width, x + w, y + quadH);
                    if (!lower.visible || lower.id != id) {
                        canExpand = false;
                        break;
                    }
                }
                if (canExpand) ++quadH;
            }

            // Clear scanned area
            for (int hy = 0; hy < quadH; ++hy) {
                for (int wx = 0; wx < quadW; ++wx) {
                    maskAt(mask, width, x + wx, y + hy).visible = false;
                }
            }

            emit(out, fixedCoord, x, y, quadW, quadH, id, face);
            x += quadW;
        }
    }
}

// Fast quad emission with AO + UVs (you already have this — keep yours or use this optimized version)
static void emitQuad(
    std::vector<ChunkVertex>& out,
    int fixed, int u, int v, int w, int h,
    uint8_t id, BlockFace face
) {
    auto& db = BlockDatabase::instance();
    const auto& fd = FACE_DATA[static_cast<int>(face)];
    const auto& verts = fd.vertices;
    const glm::vec3 normal = fd.normal;

    // Get texture coords (your existing logic)
    const auto [uvMin, uvMax] = db.getBlockFaceUV(id, face);

    // Simple AO (you can plug your full AO here)
    float ao[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    // Push 6 vertices (2 triangles)
    for (int i = 0; i < 6; ++i) {
        glm::vec3 pos = verts[i];
        if (face == BlockFace::Top || face == BlockFace::Bottom) {
            pos.x = u + (i == 0 || i == 5 ? w : 0);
            pos.z = v + (i == 2 || i == 3 ? h : 0);
            pos.y = fixed + (face == BlockFace::Top ? 1 : 0);
        } else if (face == BlockFace::North || face == BlockFace::South) {
            pos.x = u + (i == 0 || i == 5 ? w : 0);
            pos.y = v + (i == 2 || i == 3 ? h : 0);
            pos.z = fixed + (face == BlockFace::South ? 1 : 0);
        } else {
            pos.z = u + (i == 0 || i == 5 ? w : 0);
            pos.y = v + (i == 2 || i == 3 ? h : 0);
            pos.x = fixed + (face == BlockFace::East ? 1 : 0);
        }

        glm::vec2 uv = (i == 0 || i == 3 || i == 5) ? uvMin : uvMax;
        if (i == 1 || i == 4) uv.x = uvMax.x;
        if (i == 2) uv = uvMax;

        float aoVal = ao[i % 4];

        out.push_back({pos, normal, uv, glm::vec2(w, h), aoVal});
    }
}

void buildGreedyMesh(const Chunk& chunk, const MinecraftApp& app, std::vector<ChunkVertex>& outVertices) {
    outVertices.clear();
    outVertices.reserve(32 * 1024);

    const auto& db = BlockDatabase::instance();
    const glm::ivec3& cp = chunk.chunkPos;
    const int baseX = cp.x * Chunk::CHUNK_SIZE;
    const int baseZ = cp.z * Chunk::CHUNK_SIZE;

    auto getWorldBlock = [&app](int x, int y, int z) -> uint8_t {
        return app.getBlock(glm::ivec3(x, y, z));
    };

    // Reusable mask buffers
    alignas(64) static Mask maskXY[Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE];
    alignas(64) static Mask maskXZ[Chunk::CHUNK_SIZE * Chunk::CHUNK_HEIGHT];
    alignas(64) static Mask maskZY[Chunk::CHUNK_SIZE * Chunk::CHUNK_HEIGHT];

    // === Y AXIS (Top/Bottom) ===
    for (int y = 0; y < Chunk::CHUNK_HEIGHT; ++y) {
        bool hasVisible = false;
        for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
            for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
                uint8_t id = chunk.getBlock(x, y, z);
                uint8_t neighbor = (y < Chunk::CHUNK_HEIGHT - 1)
                    ? chunk.getBlock(x, y + 1, z)
                    : getWorldBlock(baseX + x, y + 1, baseZ + z);

                bool vis = db.isOpaque(id) && !db.isOpaque(neighbor);
                maskXY[z * Chunk::CHUNK_SIZE + x] = {id, vis};
                hasVisible |= vis;
            }
        }
        if (hasVisible) {
            greedyScan2D(maskXY, Chunk::CHUNK_SIZE, Chunk::CHUNK_SIZE,
                emitQuad, BlockFace::Top, y, outVertices);
        }

        // Bottom pass (reuse mask)
        hasVisible = false;
        for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
            for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
                uint8_t id = chunk.getBlock(x, y, z);
                uint8_t neighbor = (y > 0) ? chunk.getBlock(x, y - 1, z)
                                          : getWorldBlock(baseX + x, y - 1, baseZ + z);
                bool vis = db.isOpaque(id) && !db.isOpaque(neighbor);
                maskXY[z * Chunk::CHUNK_SIZE + x] = {id, vis};
                hasVisible |= vis;
            }
        }
        if (hasVisible) {
            greedyScan2D(maskXY, Chunk::CHUNK_SIZE, Chunk::CHUNK_SIZE,
                emitQuad, BlockFace::Bottom, y, outVertices);
        }
    }

    // === Z AXIS (North/South) ===
    for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
        bool hasVisible = false;
        for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
            for (int y = 0; y < Chunk::CHUNK_HEIGHT; ++y) {
                uint8_t id = chunk.getBlock(x, y, z);
                uint8_t neighbor = (z < Chunk::CHUNK_SIZE - 1)
                    ? chunk.getBlock(x, y, z + 1)
                    : getWorldBlock(baseX + x, y, baseZ + z + 1);
                bool vis = db.isOpaque(id) && !db.isOpaque(neighbor);
                maskXZ[y * Chunk::CHUNK_SIZE + x] = {id, vis};
                hasVisible |= vis;
            }
        }
        if (hasVisible) {
            greedyScan2D(maskXZ, Chunk::CHUNK_SIZE, Chunk::CHUNK_HEIGHT,
                emitQuad, BlockFace::South, z, outVertices);
        }

        // North
        hasVisible = false;
        for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
            for (int y = 0; y < Chunk::CHUNK_HEIGHT; ++y) {
                uint8_t id = chunk.getBlock(x, y, z);
                uint8_t neighbor = (z > 0) ? chunk.getBlock(x, y, z - 1)
                                          : getWorldBlock(baseX + x, y, baseZ + z - 1);
                bool vis = db.isOpaque(id) && !db.isOpaque(neighbor);
                maskXZ[y * Chunk::CHUNK_SIZE + x] = {id, vis};
                hasVisible |= vis;
            }
        }
        if (hasVisible) {
            greedyScan2D(maskXZ, Chunk::CHUNK_SIZE, Chunk::CHUNK_HEIGHT,
                emitQuad, BlockFace::North, z, outVertices);
        }
    }

    // === X AXIS (East/West) ===
    for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
        bool hasVisible = false;
        for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
            for (int y = 0; y < Chunk::CHUNK_HEIGHT; ++y) {
                uint8_t id = chunk.getBlock(x, y, z);
                uint8_t neighbor = (x < Chunk::CHUNK_SIZE - 1)
                    ? chunk.getBlock(x + 1, y, z)
                    : getWorldBlock(baseX + x + 1, y, baseZ + z);
                bool vis = db.isOpaque(id) && !db.isOpaque(neighbor);
                maskZY[y * Chunk::CHUNK_SIZE + z] = {id, vis};
                hasVisible |= vis;
            }
        }
        if (hasVisible) {
            greedyScan2D(maskZY, Chunk::CHUNK_SIZE, Chunk::CHUNK_HEIGHT,
                emitQuad, BlockFace::East, x, outVertices);
        }

        // West
        hasVisible = false;
        for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
            for (int y = 0; y < Chunk::CHUNK_HEIGHT; ++y) {
                uint8_t id = chunk.getBlock(x, y, z);
                uint8_t neighbor = (x > 0) ? chunk.getBlock(x - 1, y, z)
                                          : getWorldBlock(baseX + x - 1, y, baseZ + z);
                bool vis = db.isOpaque(id) && !db.isOpaque(neighbor);
                maskZY[y * Chunk::CHUNK_SIZE + z] = {id, vis};
                hasVisible |= vis;
            }
        }
        if (hasVisible) {
            greedyScan2D(maskZY, Chunk::CHUNK_SIZE, Chunk::CHUNK_HEIGHT,
                emitQuad, BlockFace::West, x, outVertices);
        }
    }
}

} // namespace Meshing