// Noise.h
#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <cpp/FastNoiseLite.h>
#include <memory>

/**
 * Tuned for Tectonic/Terralith-quality terrain (2025 best practices)
 */
struct TerrainParams {
    // Continentalness - tectonic plate scale
    int continentalnessOctaves = 6;
    float continentalnessLacunarity = 2.0f;
    float continentalnessGain = 0.5f;
    float continentalnessFrequency = 0.00022f;   // huge continents

    // Erosion
    int erosionOctaves = 6;
    float erosionLacunarity = 2.0f;
    float erosionGain = 0.5f;
    float erosionFrequency = 0.0011f;

    // Peaks/Valleys → ridged multifractal (sharp tectonic ridges)
    int peaksValleysOctaves = 7;
    float peaksValleysLacunarity = 2.15f;
    float peaksValleysGain = 0.7f;        // high gain = knife-edge ridges
    float peaksValleysFrequency = 0.0022f;

    // Domain warp - progressive (earth-like continents)
    int domainWarpOctaves = 6;
    float domainWarpFrequency = .0015f;
    float domainWarpAmplitude = 250.0f;  // 200-300 is the 2024-2025 sweet spot

    // Detail
    int detailOctaves = 4;
    float detailLacunarity = 2.0f;
    float detailGain = 0.5f;
    float detailFrequency = 0.016f;

    // Height multipliers - dramatic verticality
    float oceanDepthMultiplier = 120.0f;
    float beachHeightMultiplier = 35.0f;
    float landHeightMultiplier = 160.0f;
    float mountainHeightMultiplier = 350.0f;   // proper 300-400 block peaks
    float hillHeightMultiplier = 45.0f;
    float detailHeightMultiplier = 12.0f;

    int waterLevel = 63;
};

class TerrainNoise {
public:
    explicit TerrainNoise(uint32_t seed = 1337u) : m_seed(seed) {
        initializeNoiseGenerators();
    }

    TerrainParams& getParams() { return m_params; }
    const TerrainParams& getParams() const { return m_params; }
    void updateNoiseGenerators() { initializeNoiseGenerators(); }

    // NEW: shared method so Surface.cpp and sampleTerrainHeight use identical warp
    void applyDomainWarp(float& x, float& z) const {
        FastNoiseLite warpCopy(*m_domainWarp);
        warpCopy.DomainWarp(x, z);
    }

    float sampleContinentalness(float x, float z) const {
        float v = m_continentalness->GetNoise(x, z);
        return (v + 1.0f) * 0.5f;
    }

    float sampleErosion(float x, float z) const {
        float v = m_erosion->GetNoise(x, z);
        return (v + 1.0f) * 0.5f;
    }

    float samplePeaksValleys(float x, float z) const {
        float v = m_peaksValleys->GetNoise(x, z);
        return (v + 1.0f) * 0.5f;
    }

    float sampleDetail(float x, float z) const {
        float v = m_detail->GetNoise(x, z);
        return (v + 1.0f) * 0.5f;
    }

    float sampleTerrainHeight(float worldX, float worldZ) const {
        float warpedX = worldX;
        float warpedZ = worldZ;
        applyDomainWarp(warpedX, warpedZ);

        float c = sampleContinentalness(warpedX, warpedZ);
        float e = sampleErosion(warpedX, warpedZ);
        float pv = samplePeaksValleys(warpedX, warpedZ);
        float detail = sampleDetail(worldX, worldZ);  // fine detail stays unwarped

        float height = 0.0f;

        if (c < 0.45f) { // ocean
            float f = (0.45f - c) / 0.45f;
            height -= pow(f, 1.4f) * m_params.oceanDepthMultiplier;
        } else if (c < 0.55f) { // beach
            float f = (c - 0.45f) / 0.1f;
            height += pow(f, 0.7f) * m_params.beachHeightMultiplier;
        } else { // land
            float f = (c - 0.55f) / 0.45f;
            height += pow(f, 0.75f) * m_params.landHeightMultiplier;
        }

        // Sharp tectonic mountains
        float mountain = 0.0f;
        if (e < 0.45f && c > 0.55f) {
            mountain = (0.45f - e) / 0.45f;
            mountain = mountain * mountain; // quadratic = very peaked
        }

        float local = (pv - 0.5f) * 2.0f; // -1..1

        height += mountain * m_params.mountainHeightMultiplier * (local * 0.5f + 0.5f);

        if (mountain < 0.5f) {
            height += local * m_params.hillHeightMultiplier * (1.0f - mountain * 2.0f);
        }

        height += (detail - 0.5f) * m_params.detailHeightMultiplier;

        return height;
    }

private:
    void initializeNoiseGenerators() {
        m_continentalness = std::make_unique<FastNoiseLite>(m_seed);
        m_continentalness->SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        m_continentalness->SetFractalType(FastNoiseLite::FractalType_FBm);
        m_continentalness->SetFractalOctaves(m_params.continentalnessOctaves);
        m_continentalness->SetFractalLacunarity(m_params.continentalnessLacunarity);
        m_continentalness->SetFractalGain(m_params.continentalnessGain);
        m_continentalness->SetFrequency(m_params.continentalnessFrequency);

        m_erosion = std::make_unique<FastNoiseLite>(m_seed + 1);
        m_erosion->SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        m_erosion->SetFractalType(FastNoiseLite::FractalType_FBm);
        m_erosion->SetFractalOctaves(m_params.erosionOctaves);
        m_erosion->SetFractalLacunarity(m_params.erosionLacunarity);
        m_erosion->SetFractalGain(m_params.erosionGain);
        m_erosion->SetFrequency(m_params.erosionFrequency);

        // Ridged multifractal mountains
        m_peaksValleys = std::make_unique<FastNoiseLite>(m_seed + 2);
        m_peaksValleys->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        m_peaksValleys->SetFractalType(FastNoiseLite::FractalType_Ridged);
        m_peaksValleys->SetFractalOctaves(m_params.peaksValleysOctaves);
        m_peaksValleys->SetFractalLacunarity(m_params.peaksValleysLacunarity);
        m_peaksValleys->SetFractalGain(m_params.peaksValleysGain);
        m_peaksValleys->SetFrequency(m_params.peaksValleysFrequency);

        // Progressive domain warp - the current gold standard
        m_domainWarp = std::make_unique<FastNoiseLite>(m_seed + 3);
        m_domainWarp->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        m_domainWarp->SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);
        m_domainWarp->SetFractalType(FastNoiseLite::FractalType_DomainWarpProgressive);
        m_domainWarp->SetFractalOctaves(m_params.domainWarpOctaves);
        m_domainWarp->SetFrequency(m_params.domainWarpFrequency);
        m_domainWarp->SetDomainWarpAmp(m_params.domainWarpAmplitude);  // <--- key parameter

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