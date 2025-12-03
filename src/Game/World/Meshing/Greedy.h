#pragma once

#include <vector>
#include <glm/glm.hpp>

class Chunk;
class MinecraftApp;

struct ChunkVertex;

namespace Meshing {

    /**
     * Build a greedy-meshed vertex buffer for a chunk.
     *
     * - Merges coplanar faces of the same block ID into large quads.
     * - Queries neighboring chunks via MinecraftApp for correct visibility.
     * - Writes triangles into outVertices (position, normal, texCoord, ao).
     *
     * NOTE: UVs currently stretch over the merged quad instead of tiling.
     * That keeps shaders simple; we can switch to tiled UVs later if needed.
     */
    void buildGreedyMesh(
        const Chunk& chunk,
        const MinecraftApp& app,
        std::vector<ChunkVertex>& outVertices
    );

} // namespace Meshing
