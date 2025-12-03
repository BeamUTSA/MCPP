// Noise.h
#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <FastNoiseLite.h>
#include <memory>

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
        return m_domainWarp->GetNoise(x * 0.3f, z * 0.3f) * 32.0f;
    }

    /**
     * Sample domain warp offset for Z coordinate.
     */
    float sampleDomainWarpZ(float x, float z) const {
        // Use slightly offset seed for different warp pattern
        return m_domainWarp->GetNoise(x * 0.3f + 1000.0f, z * 0.3f + 1000.0f) * 32.0f;
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
            baseHeight = (continentalness - 0.45f) * 40.0f; // -18 to 0
        } else if (continentalness < 0.55f) {
            // Beach/shore - slight elevation
            baseHeight = (continentalness - 0.45f) * 20.0f; // 0 to 2
        } else {
            // Land - rising terrain
            baseHeight = (continentalness - 0.55f) * 60.0f; // 0 to 27
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
        totalHeight += mountainFactor * 80.0f * (localHeight * 0.5f + 0.5f); // 0 to 80 blocks for mountains

        // Add local variation (smaller scale)
        if (mountainFactor < 0.5f) {
            // In non-mountainous areas, add gentle rolling hills
            totalHeight += localHeight * 8.0f * (1.0f - mountainFactor);
        }

        // Add fine detail
        totalHeight += (detail - 0.5f) * 3.0f;

        return totalHeight;
    }

private:
    void initializeNoiseGenerators() {
        // Continentalness - large scale, smooth
        m_continentalness = std::make_unique<FastNoiseLite>(m_seed);
        m_continentalness->SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        m_continentalness->SetFractalType(FastNoiseLite::FractalType_FBm);
        m_continentalness->SetFractalOctaves(4);
        m_continentalness->SetFractalLacunarity(2.0f);
        m_continentalness->SetFractalGain(0.5f);
        m_continentalness->SetFrequency(0.0008f); // Very large scale

        // Erosion - medium scale
        m_erosion = std::make_unique<FastNoiseLite>(m_seed + 1);
        m_erosion->SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        m_erosion->SetFractalType(FastNoiseLite::FractalType_FBm);
        m_erosion->SetFractalOctaves(5);
        m_erosion->SetFractalLacunarity(2.0f);
        m_erosion->SetFractalGain(0.5f);
        m_erosion->SetFrequency(0.002f);

        // Peaks and valleys - local variation
        m_peaksValleys = std::make_unique<FastNoiseLite>(m_seed + 2);
        m_peaksValleys->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        m_peaksValleys->SetFractalType(FastNoiseLite::FractalType_FBm);
        m_peaksValleys->SetFractalOctaves(4);
        m_peaksValleys->SetFractalLacunarity(2.0f);
        m_peaksValleys->SetFractalGain(0.6f);
        m_peaksValleys->SetFrequency(0.005f);

        // Domain warp - for organic shapes
        m_domainWarp = std::make_unique<FastNoiseLite>(m_seed + 3);
        m_domainWarp->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        m_domainWarp->SetFractalType(FastNoiseLite::FractalType_FBm);
        m_domainWarp->SetFractalOctaves(3);
        m_domainWarp->SetFrequency(0.003f);

        // Detail - fine-scale features
        m_detail = std::make_unique<FastNoiseLite>(m_seed + 4);
        m_detail->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        m_detail->SetFractalType(FastNoiseLite::FractalType_FBm);
        m_detail->SetFractalOctaves(3);
        m_detail->SetFractalLacunarity(2.0f);
        m_detail->SetFractalGain(0.5f);
        m_detail->SetFrequency(0.01f);
    }

    uint32_t m_seed;
    std::unique_ptr<FastNoiseLite> m_continentalness;
    std::unique_ptr<FastNoiseLite> m_erosion;
    std::unique_ptr<FastNoiseLite> m_peaksValleys;
    std::unique_ptr<FastNoiseLite> m_domainWarp;
    std::unique_ptr<FastNoiseLite> m_detail;
};
