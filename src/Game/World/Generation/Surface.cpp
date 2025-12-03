// Surface.cpp
#include "Game/World/Generation/Surface.h"
#include "Game/World/Generation/Noise.h"

#include "Game/World/Block/BlockDatabase.h"

class ImprovedSurface : public Surface {
public:
    explicit ImprovedSurface(uint32_t seed = 1337u);

    SurfaceSample sampleColumn(int worldX, int worldZ) const override;

    // Override to expose TerrainNoise for parameter tweaking
    TerrainNoise* getTerrainNoise() override { return &m_noise; }
    const TerrainNoise* getTerrainNoise() const override { return &m_noise; }

private:
    TerrainNoise m_noise;

    // Cached block IDs
    uint8_t m_grassId;
    uint8_t m_dirtId;
    uint8_t m_stoneId;
    uint8_t m_sandId;
    uint8_t m_waterId;
    uint8_t m_snowId;
};

ImprovedSurface::ImprovedSurface(uint32_t seed)
    : m_noise(seed)
{
    // Grab block IDs once (assuming names exist in your registry)
    auto& db = MCPP::BlockDatabase::instance();
    m_grassId = db.getBlockByName("Grass")->id;
    m_dirtId  = db.getBlockByName("Dirt")->id;
    m_stoneId = db.getBlockByName("Stone")->id;
    m_sandId  = db.getBlockByName("Sand")->id;

    // Try to get water and snow, fallback to stone if not available
    auto* waterBlock = db.getBlockByName("Water");
    m_waterId = waterBlock ? waterBlock->id : m_stoneId;

    auto* snowBlock = db.getBlockByName("Snow");
    m_snowId = snowBlock ? snowBlock->id : m_grassId;
}

std::unique_ptr<Surface> createDefaultSurface(uint32_t seed) {
    return std::make_unique<ImprovedSurface>(seed);
}


SurfaceSample ImprovedSurface::sampleColumn(int worldX, int worldZ) const {
    // Use the advanced terrain noise system
    float terrainHeight = m_noise.sampleTerrainHeight(
        static_cast<float>(worldX),
        static_cast<float>(worldZ)
    );

    // Add base water level to convert to absolute height
    const int WATER_LEVEL = m_noise.getParams().waterLevel;
    int absoluteHeight = WATER_LEVEL + static_cast<int>(terrainHeight);

    // Clamp to reasonable range (0 to 255 for standard Minecraft-like world)
    absoluteHeight = std::max(0, std::min(255, absoluteHeight));

    // Get individual noise values for biome-like decisions
    float warpedX = worldX + m_noise.sampleDomainWarpX(worldX, worldZ);
    float warpedZ = worldZ + m_noise.sampleDomainWarpZ(worldX, worldZ);
    float continentalness = m_noise.sampleContinentalness(warpedX, warpedZ);
    float erosion = m_noise.sampleErosion(warpedX, warpedZ);

    SurfaceSample s{};
    s.height = absoluteHeight;

    // Determine surface blocks based on terrain type
    if (absoluteHeight < WATER_LEVEL) {
        // Underwater - use sand on ocean floor
        s.topBlock = m_sandId;
        s.fillerBlock = m_sandId;
        s.stoneBlock = m_stoneId;
    }
    else if (absoluteHeight < WATER_LEVEL + 3) {
        // Beach areas - sandy shores
        s.topBlock = m_sandId;
        s.fillerBlock = m_sandId;
        s.stoneBlock = m_stoneId;
    }
    else if (absoluteHeight > 120 && erosion < 0.3f) {
        // High mountain peaks - snow-capped
        s.topBlock = m_snowId;
        s.fillerBlock = m_stoneId;
        s.stoneBlock = m_stoneId;
    }
    else if (erosion < 0.35f && continentalness > 0.55f) {
        // Mountain areas - exposed stone
        s.topBlock = m_stoneId;
        s.fillerBlock = m_stoneId;
        s.stoneBlock = m_stoneId;
    }
    else {
        // Normal land - grass and dirt
        s.topBlock = m_grassId;
        s.fillerBlock = m_dirtId;
        s.stoneBlock = m_stoneId;
    }

    return s;
}
