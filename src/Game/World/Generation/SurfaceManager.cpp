// SurfaceManager.cpp
#include "Game/World/Generation/SurfaceManager.h"
#include "Game/World/Generation/Surface.h"

// We need DefaultSurface from Surface.cpp, so add a forward or a factory.
// Easiest: declare a factory function in Surface.cpp and extern here.

// In Surface.cpp, at bottom, add:
//   std::unique_ptr<Surface> createDefaultSurface(uint32_t seed) {
//       return std::make_unique<DefaultSurface>(seed);
//   }

std::unique_ptr<Surface> createDefaultSurface(uint32_t seed); // forward

SurfaceManager::SurfaceManager(uint32_t worldSeed) {
    m_defaultSurface = createDefaultSurface(worldSeed);
}

SurfaceSample SurfaceManager::sampleColumn(int worldX, int worldZ) const {
    return m_defaultSurface->sampleColumn(worldX, worldZ);
}
