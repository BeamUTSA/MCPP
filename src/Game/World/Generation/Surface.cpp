// Surface.cpp
#include "Game/World/Generation/Surface.h"
#include "Game/World/Generation/Noise.h"

#include "Game/World/Block/BlockDatabase.h"  // to get block IDs by name, if you like

class DefaultSurface : public Surface {
public:
    explicit DefaultSurface(uint32_t seed = 1337u);

    SurfaceSample sampleColumn(int worldX, int worldZ) const override;

private:
    uint32_t m_seed;

    // Tunables
    int   m_baseHeight     = 48;    // "sea level"
    float m_heightScale    = 8.0f; // amplitude of hills
    int   m_octaves        = 4;
    float m_frequency      = 0.2f;
    float m_persistence    = 0.5f;

    // Cached block IDs
    uint8_t m_grassId;
    uint8_t m_dirtId;
    uint8_t m_stoneId;
};

DefaultSurface::DefaultSurface(uint32_t seed)
    : m_seed(seed)
{
    // Grab block IDs once (assuming names exist in your registry)
    auto& db = MCPP::BlockDatabase::instance();
    m_grassId = db.getBlockByName("Grass")->id;
    m_dirtId  = db.getBlockByName("Dirt")->id;
    m_stoneId = db.getBlockByName("Stone")->id;
}

std::unique_ptr<Surface> createDefaultSurface(uint32_t seed) {
    return std::make_unique<DefaultSurface>(seed);
}


SurfaceSample DefaultSurface::sampleColumn(int worldX, int worldZ) const {
    // Basic FBM-based height
    float hNoise = fbm2D(worldX, worldZ, m_octaves, m_frequency, m_persistence, m_seed);
    int   height = m_baseHeight + static_cast<int>(hNoise * m_heightScale);

    SurfaceSample s{};
    s.height       = height;
    s.topBlock     = m_grassId;
    s.fillerBlock  = m_dirtId;
    s.stoneBlock   = m_stoneId;
    return s;
}
