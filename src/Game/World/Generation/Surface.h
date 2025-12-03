// Surface.h
#pragma once

#include <cstdint>
#include <glm/glm.hpp>

// Forward declare if you want later hooks for biomes, etc.
struct SurfaceSample {
    int     height;       // highest solid Y for terrain column
    uint8_t topBlock;     // grass, sand, etc.
    uint8_t fillerBlock;  // dirt, sand, etc.
    uint8_t stoneBlock;   // base stone layer
};

/**
 * Base interface for a "surface generator".
 *
 * Given world X/Z and maybe some params/seed, returns how the terrain should
 * look at that column.
 */
class Surface {
public:
    virtual ~Surface() = default;

    /**
     * Sample the terrain height and layers at a given world X/Z.
     */
    virtual SurfaceSample sampleColumn(int worldX, int worldZ) const = 0;
};
