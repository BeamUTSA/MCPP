#include "Game/World/Meshing/Greedy.h"

#include "Game/World/Generation/Chunk.h"
#include "Game/MinecraftApp.h"
#include "Game/World/Block/BlockDatabase.h"

#include <functional>
#include <vector>
#include <array>

using MCPP::BlockDatabase;
using MCPP::BlockFace;
using MCPP::UVCoords;

namespace {

// Canonical face normals (must match BlockFace enum order)
constexpr std::array<glm::vec3, 6> FACE_NORMALS = {{
    { 0.0f,  1.0f,  0.0f},  // Top (Y+)
    { 0.0f, -1.0f,  0.0f},  // Bottom (Y-)
    { 0.0f,  0.0f, -1.0f},  // North (Z-)
    { 0.0f,  0.0f,  1.0f},  // South (Z+)
    { 1.0f,  0.0f,  0.0f},  // East  (X+)
    {-1.0f,  0.0f,  0.0f},  // West  (X-)
}};

// Canonical 1x1 cube face vertices in local [0,1] space.
// Order is 6 vertices (two triangles) per face, CCW for front-facing side.
constexpr std::array<std::array<glm::vec3, 6>, 6> FACE_TEMPLATE_POS = {{
    // Top (Y+)
    {{
        {1,1,1}, {1,1,0}, {0,1,0},
        {0,1,0}, {0,1,1}, {1,1,1}
    }},
    // Bottom (Y-)
    {{
        {0,0,0}, {1,0,0}, {1,0,1},
        {1,0,1}, {0,0,1}, {0,0,0}
    }},
    // North (Z-)
    {{
        {1,0,0}, {0,0,0}, {0,1,0},
        {0,1,0}, {1,1,0}, {1,0,0}
    }},
    // South (Z+)
    {{
        {1,1,1}, {0,1,1}, {0,0,1},
        {0,0,1}, {1,0,1}, {1,1,1}
    }},
    // East (X+)
    {{
        {1,1,1}, {1,0,1}, {1,0,0},
        {1,0,0}, {1,1,0}, {1,1,1}
    }},
    // West (X-)
    {{
        {0,1,1}, {0,1,0}, {0,0,0},
        {0,0,0}, {0,0,1}, {0,1,1}
    }},
}};

struct MaskCell {
    bool visible = false;
    uint8_t id   = 0;
};

inline MaskCell& maskAt(std::vector<MaskCell>& mask, int width, int x, int y) {
    return mask[y * width + x];
}

// Greedy 2D scan: turns mask into rectangles
void greedyScan2D(
    int width,
    int height,
    std::vector<MaskCell>& mask,
    const std::function<void(int, int, int, int, uint8_t)>& emit
) {
    for (int y = 0; y < height; ++y) {
        int x = 0;
        while (x < width) {
            MaskCell& cell = maskAt(mask, width, x, y);
            if (!cell.visible) {
                ++x;
                continue;
            }

            const uint8_t id = cell.id;

            // Width
            int quadWidth = 1;
            while (x + quadWidth < width) {
                MaskCell& rightCell = maskAt(mask, width, x + quadWidth, y);
                if (!rightCell.visible || rightCell.id != id) break;
                ++quadWidth;
            }

            // Height
            int quadHeight = 1;
            bool done = false;
            while (y + quadHeight < height && !done) {
                for (int k = 0; k < quadWidth; ++k) {
                    MaskCell& rowCell = maskAt(mask, width, x + k, y + quadHeight);
                    if (!rowCell.visible || rowCell.id != id) {
                        done = true;
                        break;
                    }
                }
                if (!done) ++quadHeight;
            }

            // Mark consumed
            for (int dy = 0; dy < quadHeight; ++dy)
                for (int dx = 0; dx < quadWidth; ++dx)
                    maskAt(mask, width, x + dx, y + dy).visible = false;

            emit(x, y, quadWidth, quadHeight, id);
            x += quadWidth;
        }
    }
}

// Block lookup that handles neighbors in other chunks
uint8_t getBlockWithNeighbors(const Chunk& chunk, const MinecraftApp& app, int x, int y, int z) {
    if (x >= 0 && x < Chunk::CHUNK_SIZE &&
        y >= 0 && y < Chunk::CHUNK_HEIGHT &&
        z >= 0 && z < Chunk::CHUNK_SIZE) {
        return chunk.getBlock(x, y, z);
    }

    glm::vec3 worldPos = chunk.getBlockWorldPosition(x, y, z);
    return app.getBlock(glm::ivec3(
        static_cast<int>(worldPos.x),
        static_cast<int>(worldPos.y),
        static_cast<int>(worldPos.z)
    ));
}

// Emit a single rectangle (two triangles) into the vertex buffer.
// planeIndex: slice index along the axis (y for top/bottom, z for north/south, x for east/west)
// (x,y)      : origin in the 2D mask for that slice
// w,h        : extent in mask space
void emitQuad(
    std::vector<ChunkVertex>& outVertices,
    BlockFace face,
    int planeIndex,
    int x,
    int y,
    int w,
    int h,
    uint8_t blockId
) {
    const auto& db = BlockDatabase::instance();
    UVCoords uv = db.getBlockFaceUV(blockId, face);

    const glm::vec2 uvMin = uv.min;
    const glm::vec2 uvMax = uv.max;
    const glm::vec3 normal = FACE_NORMALS[static_cast<size_t>(face)];
    const auto& templateVerts = FACE_TEMPLATE_POS[static_cast<size_t>(face)];

    for (const glm::vec3& c : templateVerts) {
        float cx = c.x;
        float cy = c.y;
        float cz = c.z;

        glm::vec3 pos;

        switch (face) {
            case BlockFace::Top: {
                // Plane at y+1, width along X, height along Z
                float baseY = static_cast<float>(planeIndex + 1);
                float baseX = static_cast<float>(x);
                float baseZ = static_cast<float>(y);
                pos = {
                    baseX + (cx > 0.5f ? w : 0.0f),
                    baseY,
                    baseZ + (cz > 0.5f ? h : 0.0f)
                };
                break;
            }
            case BlockFace::Bottom: {
                // Plane at y, width along X, height along Z
                float baseY = static_cast<float>(planeIndex);
                float baseX = static_cast<float>(x);
                float baseZ = static_cast<float>(y);
                pos = {
                    baseX + (cx > 0.5f ? w : 0.0f),
                    baseY,
                    baseZ + (cz > 0.5f ? h : 0.0f)
                };
                break;
            }
            case BlockFace::North: {
                // Plane at z, width along X, height along Y
                float baseZ = static_cast<float>(planeIndex);
                float baseX = static_cast<float>(x);
                float baseY = static_cast<float>(y);
                pos = {
                    baseX + (cx > 0.5f ? w : 0.0f),
                    baseY + (cy > 0.5f ? h : 0.0f),
                    baseZ
                };
                break;
            }
            case BlockFace::South: {
                // Plane at z+1, width along X, height along Y
                float baseZ = static_cast<float>(planeIndex + 1);
                float baseX = static_cast<float>(x);
                float baseY = static_cast<float>(y);
                pos = {
                    baseX + (cx > 0.5f ? w : 0.0f),
                    baseY + (cy > 0.5f ? h : 0.0f),
                    baseZ
                };
                break;
            }
            case BlockFace::East: {
                // Plane at x+1, width along Z, height along Y
                float baseX = static_cast<float>(planeIndex + 1);
                float baseZ = static_cast<float>(x);
                float baseY = static_cast<float>(y);
                pos = {
                    baseX,
                    baseY + (cy > 0.5f ? h : 0.0f),
                    baseZ + (cz > 0.5f ? w : 0.0f)
                };
                break;
            }
            case BlockFace::West: {
                // Plane at x, width along Z, height along Y
                float baseX = static_cast<float>(planeIndex);
                float baseZ = static_cast<float>(x);
                float baseY = static_cast<float>(y);
                pos = {
                    baseX,
                    baseY + (cy > 0.5f ? h : 0.0f),
                    baseZ + (cz > 0.5f ? w : 0.0f)
                };
                break;
            }
        }

        // UV mapping:
        // For now we just stretch the texture over the merged quad.
        float uFactor = 0.0f;
        float vFactor = 0.0f;

        switch (face) {
            case BlockFace::Top:
            case BlockFace::Bottom:
                // u along X, v along Z
                uFactor = (cx > 0.5f) ? 1.0f : 0.0f;
                vFactor = (cz > 0.5f) ? 1.0f : 0.0f;
                break;
            case BlockFace::North:
            case BlockFace::South:
                // u along X, v along Y
                uFactor = (cx > 0.5f) ? 1.0f : 0.0f;
                vFactor = (cy > 0.5f) ? 1.0f : 0.0f;
                break;
            case BlockFace::East:
            case BlockFace::West:
                // u along Z, v along Y
                uFactor = (cz > 0.5f) ? 1.0f : 0.0f;
                vFactor = (cy > 0.5f) ? 1.0f : 0.0f;
                break;
        }

        glm::vec2 uvCoord{
            uvMin.x + (uvMax.x - uvMin.x) * uFactor,
            uvMin.y + (uvMax.y - uvMin.y) * vFactor
        };

        ChunkVertex vert;
        vert.position = pos;
        vert.normal   = normal;
        vert.texCoord = uvCoord;
        vert.ao       = 1.0f; // AO can be reintroduced later

        outVertices.push_back(vert);
    }
}

} // anonymous namespace

namespace Meshing {

void buildGreedyMesh(
    const Chunk& chunk,
    const MinecraftApp& app,
    std::vector<ChunkVertex>& outVertices
) {
    outVertices.clear();
    outVertices.reserve(1024); // heuristic, will grow as needed

    auto& db = BlockDatabase::instance();

    // ---------- +Y (Top) & -Y (Bottom) ----------
    {
        const int width  = Chunk::CHUNK_SIZE;  // X
        const int height = Chunk::CHUNK_SIZE;  // Z
        std::vector<MaskCell> mask(width * height);

        // TOP (Y+)
        for (int y = 0; y < Chunk::CHUNK_HEIGHT; ++y) {
            for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
                for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
                    MaskCell& cell = maskAt(mask, width, x, z);
                    uint8_t curId   = getBlockWithNeighbors(chunk, app, x, y, z);
                    uint8_t aboveId = getBlockWithNeighbors(chunk, app, x, y + 1, z);

                    bool visible = db.isOpaque(curId) && !db.isOpaque(aboveId);
                    cell.visible = visible;
                    cell.id      = curId;
                }
            }

            greedyScan2D(
                width,
                height,
                mask,
                [&](int mx, int mz, int w, int h, uint8_t id) {
                    emitQuad(outVertices, BlockFace::Top, y, mx, mz, w, h, id);
                }
            );
        }

        // BOTTOM (Y-)
        for (int y = 0; y < Chunk::CHUNK_HEIGHT; ++y) {
            for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
                for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
                    MaskCell& cell = maskAt(mask, width, x, z);
                    uint8_t curId   = getBlockWithNeighbors(chunk, app, x, y, z);
                    uint8_t belowId = getBlockWithNeighbors(chunk, app, x, y - 1, z);

                    bool visible = db.isOpaque(curId) && !db.isOpaque(belowId);
                    cell.visible = visible;
                    cell.id      = curId;
                }
            }

            greedyScan2D(
                width,
                height,
                mask,
                [&](int mx, int mz, int w, int h, uint8_t id) {
                    emitQuad(outVertices, BlockFace::Bottom, y, mx, mz, w, h, id);
                }
            );
        }
    }

    // ---------- +Z (South) & -Z (North) ----------
    {
        const int width  = Chunk::CHUNK_SIZE;   // X
        const int height = Chunk::CHUNK_HEIGHT; // Y
        std::vector<MaskCell> mask(width * height);

        // SOUTH (Z+)
        for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
            for (int y = 0; y < Chunk::CHUNK_HEIGHT; ++y) {
                for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
                    MaskCell& cell = maskAt(mask, width, x, y);
                    uint8_t curId   = getBlockWithNeighbors(chunk, app, x, y, z);
                    uint8_t frontId = getBlockWithNeighbors(chunk, app, x, y, z + 1);

                    bool visible = db.isOpaque(curId) && !db.isOpaque(frontId);
                    cell.visible = visible;
                    cell.id      = curId;
                }
            }

            greedyScan2D(
                width,
                height,
                mask,
                [&](int mx, int my, int w, int h, uint8_t id) {
                    emitQuad(outVertices, BlockFace::South, z, mx, my, w, h, id);
                }
            );
        }

        // NORTH (Z-)
        for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
            for (int y = 0; y < Chunk::CHUNK_HEIGHT; ++y) {
                for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
                    MaskCell& cell = maskAt(mask, width, x, y);
                    uint8_t curId  = getBlockWithNeighbors(chunk, app, x, y, z);
                    uint8_t backId = getBlockWithNeighbors(chunk, app, x, y, z - 1);

                    bool visible = db.isOpaque(curId) && !db.isOpaque(backId);
                    cell.visible = visible;
                    cell.id      = curId;
                }
            }

            greedyScan2D(
                width,
                height,
                mask,
                [&](int mx, int my, int w, int h, uint8_t id) {
                    emitQuad(outVertices, BlockFace::North, z, mx, my, w, h, id);
                }
            );
        }
    }

    // ---------- +X (East) & -X (West) ----------
    {
        const int width  = Chunk::CHUNK_SIZE;   // Z
        const int height = Chunk::CHUNK_HEIGHT; // Y
        std::vector<MaskCell> mask(width * height);

        // EAST (X+)
        for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
            for (int y = 0; y < Chunk::CHUNK_HEIGHT; ++y) {
                for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
                    MaskCell& cell = maskAt(mask, width, z, y);
                    uint8_t curId   = getBlockWithNeighbors(chunk, app, x, y, z);
                    uint8_t rightId = getBlockWithNeighbors(chunk, app, x + 1, y, z);

                    bool visible = db.isOpaque(curId) && !db.isOpaque(rightId);
                    cell.visible = visible;
                    cell.id      = curId;
                }
            }

            greedyScan2D(
                width,
                height,
                mask,
                [&](int mz, int my, int w, int h, uint8_t id) {
                    emitQuad(outVertices, BlockFace::East, x, mz, my, w, h, id);
                }
            );
        }

        // WEST (X-)
        for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
            for (int y = 0; y < Chunk::CHUNK_HEIGHT; ++y) {
                for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
                    MaskCell& cell = maskAt(mask, width, z, y);
                    uint8_t curId  = getBlockWithNeighbors(chunk, app, x, y, z);
                    uint8_t leftId = getBlockWithNeighbors(chunk, app, x - 1, y, z);

                    bool visible = db.isOpaque(curId) && !db.isOpaque(leftId);
                    cell.visible = visible;
                    cell.id      = curId;
                }
            }

            greedyScan2D(
                width,
                height,
                mask,
                [&](int mz, int my, int w, int h, uint8_t id) {
                    emitQuad(outVertices, BlockFace::West, x, mz, my, w, h, id);
                }
            );
        }
    }
}

} // namespace Meshing
