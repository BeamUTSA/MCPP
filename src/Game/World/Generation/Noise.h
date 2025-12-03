// Noise.h
#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <FastNoiseLite.h>
#include <memory>

/**
 * Structure to hold all terrain generation parameters for easy tweaking
 */
struct TerrainParams {
    // Continentalness parameters
    int continentalnessOctaves = 4;
    float continentalnessLacunarity = 2.0f;
    float continentalnessGain = 0.5f;
    float continentalnessFrequency = 0.0008f;

    // Erosion parameters
    int erosionOctaves = 5;
    float erosionLacunarity = 2.0f;
    float erosionGain = 0.5f;
    float erosionFrequency = 0.002f;

    // Peaks/Valleys parameters
    int peaksValleysOctaves = 4;
    float peaksValleysLacunarity = 2.0f;
    float peaksValleysGain = 0.6f;
    float peaksValleysFrequency = 0.005f;

    // Domain warp parameters
    int domainWarpOctaves = 3;
    float domainWarpFrequency = 0.003f;
    float domainWarpStrength = 32.0f;

    // Detail parameters
    int detailOctaves = 3;
    float detailLacunarity = 2.0f;
    float detailGain = 0.5f;
    float detailFrequency = 0.01f;

    // Height generation parameters
    float oceanDepthMultiplier = 40.0f;
    float beachHeightMultiplier = 20.0f;
    float landHeightMultiplier = 60.0f;
    float mountainHeightMultiplier = 80.0f;
    float hillHeightMultiplier = 8.0f;
    float detailHeightMultiplier = 3.0f;

    // Water level
    int waterLevel = 48;
};

/**
 * Advanced noise generation for smooth, realistic Minecraft/Tectonic-like terrain.
 *
 * This system uses FastNoiseLite to generate smooth Perlin/Simplex noise with:
 * - Multi-octave fractal noise (FBM)
 * - Domain warping for organic shapes
 * - Multiple noise layers (continentalness, erosion, peaks/valleys)
 */
class TerrainNoise {
public:
    explicit TerrainNoise(uint32_t seed = 1337u) : m_seed(seed) {
        initializeNoiseGenerators();
    }

    // Access to parameters for runtime editing
    TerrainParams& getParams() { return m_params; }
    const TerrainParams& getParams() const { return m_params; }

    // Call this after modifying parameters to rebuild noise generators
    void updateNoiseGenerators() { initializeNoiseGenerators(); }

    /**
     * Sample continentalness - determines ocean vs land and broad terrain shape.
     * Returns value in [0, 1] where:
     * - 0.0-0.3: Deep ocean
     * - 0.3-0.45: Ocean/coast
     * - 0.45-0.55: Beach/shore
     * - 0.55-1.0: Land (increasing distance inland)
     */
    float sampleContinentalness(float x, float z) const {
        // Large-scale noise for broad continental features
        float value = m_continentalness->GetNoise(x * 0.5f, z * 0.5f);
        // Remap from [-1, 1] to [0, 1]
        return (value + 1.0f) * 0.5f;
    }

    /**
     * Sample erosion - determines flat vs mountainous terrain.
     * Returns value in [0, 1] where:
     * - 0.0-0.3: Highly eroded, mountainous, sharp peaks
     * - 0.3-0.6: Medium erosion, hills and valleys
     * - 0.6-1.0: Low erosion, flat plains
     */
    float sampleErosion(float x, float z) const {
        float value = m_erosion->GetNoise(x * 0.8f, z * 0.8f);
        return (value + 1.0f) * 0.5f;
    }

    /**
     * Sample peaks and valleys - local height variations.
     * Returns value in [0, 1] where higher = peaks, lower = valleys.
     */
    float samplePeaksValleys(float x, float z) const {
        float value = m_peaksValleys->GetNoise(x * 1.2f, z * 1.2f);
        return (value + 1.0f) * 0.5f;
    }

    /**
     * Sample domain warp offset for X coordinate.
     * Adds organic, flowing character to terrain.
     */
    float sampleDomainWarpX(float x, float z) const {
        return m_domainWarp->GetNoise(x * 0.3f, z * 0.3f) * m_params.domainWarpStrength;
    }

    /**
     * Sample domain warp offset for Z coordinate.
     */
    float sampleDomainWarpZ(float x, float z) const {
        // Use slightly offset seed for different warp pattern
        return m_domainWarp->GetNoise(x * 0.3f + 1000.0f, z * 0.3f + 1000.0f) * m_params.domainWarpStrength;
    }

    /**
     * Sample detailed noise for small-scale terrain features.
     */
    float sampleDetail(float x, float z) const {
        float value = m_detail->GetNoise(x * 2.0f, z * 2.0f);
        return (value + 1.0f) * 0.5f;
    }

    /**
     * Combined terrain height sampling with all noise layers and domain warping.
     * Returns height value (can be negative for underwater terrain).
     */
    float sampleTerrainHeight(float worldX, float worldZ) const {
        // Apply domain warping first for organic shapes
        float warpedX = worldX + sampleDomainWarpX(worldX, worldZ);
        float warpedZ = worldZ + sampleDomainWarpZ(worldX, worldZ);

        // Sample all noise layers
        float continentalness = sampleContinentalness(warpedX, warpedZ);
        float erosion = sampleErosion(warpedX, warpedZ);
        float peaksValleys = samplePeaksValleys(warpedX, warpedZ);
        float detail = sampleDetail(worldX, worldZ);

        // Base height from continentalness
        // Ocean areas have negative height, land areas positive
        float baseHeight = 0.0f;
        if (continentalness < 0.45f) {
            // Ocean - gradually deepening
            baseHeight = (continentalness - 0.45f) * m_params.oceanDepthMultiplier;
        } else if (continentalness < 0.55f) {
            // Beach/shore - slight elevation
            baseHeight = (continentalness - 0.45f) * m_params.beachHeightMultiplier;
        } else {
            // Land - rising terrain
            baseHeight = (continentalness - 0.55f) * m_params.landHeightMultiplier;
        }

        // Mountain height based on erosion (inverted - low erosion = flat, high erosion = mountains)
        float mountainFactor = 0.0f;
        if (erosion < 0.4f && continentalness > 0.5f) {
            // Mountainous areas only on land
            mountainFactor = (0.4f - erosion) / 0.4f; // 0 to 1
            // Mountains get more dramatic with lower erosion
            mountainFactor = mountainFactor * mountainFactor; // Square for sharper peaks
        }

        // Peaks and valleys add local variation
        float localHeight = (peaksValleys - 0.5f) * 2.0f; // -1 to 1

        // Combine all factors
        float totalHeight = baseHeight;

        // Add mountain height (can be very tall)
        totalHeight += mountainFactor * m_params.mountainHeightMultiplier * (localHeight * 0.5f + 0.5f);

        // Add local variation (smaller scale)
        if (mountainFactor < 0.5f) {
            // In non-mountainous areas, add gentle rolling hills
            totalHeight += localHeight * m_params.hillHeightMultiplier * (1.0f - mountainFactor);
        }

        // Add fine detail
        totalHeight += (detail - 0.5f) * m_params.detailHeightMultiplier;

        return totalHeight;
    }

private:
    void initializeNoiseGenerators() {
        // Continentalness - large scale, smooth
        m_continentalness = std::make_unique<FastNoiseLite>(m_seed);
        m_continentalness->SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        m_continentalness->SetFractalType(FastNoiseLite::FractalType_FBm);
        m_continentalness->SetFractalOctaves(m_params.continentalnessOctaves);
        m_continentalness->SetFractalLacunarity(m_params.continentalnessLacunarity);
        m_continentalness->SetFractalGain(m_params.continentalnessGain);
        m_continentalness->SetFrequency(m_params.continentalnessFrequency);

        // Erosion - medium scale
        m_erosion = std::make_unique<FastNoiseLite>(m_seed + 1);
        m_erosion->SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        m_erosion->SetFractalType(FastNoiseLite::FractalType_FBm);
        m_erosion->SetFractalOctaves(m_params.erosionOctaves);
        m_erosion->SetFractalLacunarity(m_params.erosionLacunarity);
        m_erosion->SetFractalGain(m_params.erosionGain);
        m_erosion->SetFrequency(m_params.erosionFrequency);

        // Peaks and valleys - local variation
        m_peaksValleys = std::make_unique<FastNoiseLite>(m_seed + 2);
        m_peaksValleys->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        m_peaksValleys->SetFractalType(FastNoiseLite::FractalType_FBm);
        m_peaksValleys->SetFractalOctaves(m_params.peaksValleysOctaves);
        m_peaksValleys->SetFractalLacunarity(m_params.peaksValleysLacunarity);
        m_peaksValleys->SetFractalGain(m_params.peaksValleysGain);
        m_peaksValleys->SetFrequency(m_params.peaksValleysFrequency);

        // Domain warp - for organic shapes
        m_domainWarp = std::make_unique<FastNoiseLite>(m_seed + 3);
        m_domainWarp->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        m_domainWarp->SetFractalType(FastNoiseLite::FractalType_FBm);
        m_domainWarp->SetFractalOctaves(m_params.domainWarpOctaves);
        m_domainWarp->SetFrequency(m_params.domainWarpFrequency);

        // Detail - fine-scale features
        m_detail = std::make_unique<FastNoiseLite>(m_seed + 4);
        m_detail->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        m_detail->SetFractalType(FastNoiseLite::FractalType_FBm);
        m_detail->SetFractalOctaves(m_params.detailOctaves);
        m_detail->SetFractalLacunarity(m_params.detailLacunarity);
        m_detail->SetFractalGain(m_params.detailGain);
        m_detail->SetFrequency(m_params.detailFrequency);
    }

    uint32_t m_seed;
    TerrainParams m_params;
    std::unique_ptr<FastNoiseLite> m_continentalness;
    std::unique_ptr<FastNoiseLite> m_erosion;
    std::unique_ptr<FastNoiseLite> m_peaksValleys;
    std::unique_ptr<FastNoiseLite> m_domainWarp;
    std::unique_ptr<FastNoiseLite> m_detail;
};
