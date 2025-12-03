// Noise.h
#pragma once

#include <cstdint>
#include <glm/glm.hpp>

/**
 * Super simple, hash-based pseudo noise in [0,1].
 * Not pretty, but deterministic and fast enough to test pipeline.
 */
inline float hashNoise2D(int x, int z, uint32_t seed = 1337u) {
    uint32_t h = static_cast<uint32_t>(x) * 374761393u
               + static_cast<uint32_t>(z) * 668265263u
               + seed * 374761393u;

    h = (h ^ (h >> 13u)) * 1274126177u;
    h ^= (h >> 16u);

    // Map to [0,1]
    return (h & 0xFFFFFFu) / static_cast<float>(0xFFFFFFu);
}

/**
 * Simple fractal "noise" by combining multiple octaves of hashNoise2D.
 */
inline float fbm2D(int x, int z, int octaves, float frequency, float persistence, uint32_t seed = 1337u) {
    float total    = 0.0f;
    float maxTotal = 0.0f;
    float amp      = 1.0f;
    float freq     = frequency;

    for (int i = 0; i < octaves; ++i) {
        float nx = x * freq;
        float nz = z * freq;

        // Cast to int for now (coarse, but OK for first pass)
        total    += hashNoise2D(static_cast<int>(nx), static_cast<int>(nz), seed + i * 31u) * amp;
        maxTotal += amp;

        amp  *= persistence;
        freq *= 2.0f;
    }

    return (maxTotal > 0.0f) ? total / maxTotal : 0.0f;
}
