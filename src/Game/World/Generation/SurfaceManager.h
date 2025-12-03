// SurfaceManager.h
#pragma once

#include <memory>
#include <cstdint>
#include <glm/glm.hpp>

#include "Game/World/Generation/Surface.h"

/**
 * SurfaceManager - owns one or more Surface implementations and
 * chooses which one to use for a given chunk / world position.
 *
 * For now it's trivial: a single DefaultSurface for the whole world.
 */
class SurfaceManager {
public:
    explicit SurfaceManager(uint32_t worldSeed = 1337u);

    /**
     * Sample the terrain at a world X/Z position.
     */
    SurfaceSample sampleColumn(int worldX, int worldZ) const;

    /**
     * Get the default surface for direct access (for tweaking parameters)
     */
    Surface* getDefaultSurface() { return m_defaultSurface.get(); }
    const Surface* getDefaultSurface() const { return m_defaultSurface.get(); }

private:
    std::unique_ptr<Surface> m_defaultSurface;
};
