#include "GpuNoiseMatch.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace GpuNoiseMatch {

    static float Fract(float x) { return x - std::floor(x); }
    static glm::vec2 Fract(glm::vec2 v) { return glm::vec2(Fract(v.x), Fract(v.y)); }
    static glm::vec3 Fract(glm::vec3 v) { return glm::vec3(Fract(v.x), Fract(v.y), Fract(v.z)); }
    static float Mod(float x, float y) { return x - y * std::floor(x / y); }
    static glm::vec2 Mod(glm::vec2 v, float y) { return glm::vec2(Mod(v.x, y), Mod(v.y, y)); }
    static glm::vec3 Mod(glm::vec3 v, float y) { return glm::vec3(Mod(v.x, y), Mod(v.y, y), Mod(v.z, y)); }

    // Exact port of noise.glsl's hashSeed
    float HashSeed(float seed) {
        float wrapped = Mod(seed, 1000.0f);
        float x = Fract(std::sin(wrapped * 12.9898f) * 43758.5453f);
        return x * 1000.0f;
    }

    // Exact port of noise.glsl's permute
    static glm::vec3 Permute(glm::vec3 x) {
        return Mod(((x * 34.0f) + 1.0f) * x, 289.0f);
    }

    // Exact port of noise.glsl's simplexNoise2D
    float SimplexNoise2D(glm::vec2 v) {
        const glm::vec4 C(0.211324865405187f, 0.366025403784439f, -0.577350269189626f, 0.024390243902439f);

        glm::vec2 i = glm::floor(v + glm::dot(v, glm::vec2(C.y, C.y)));
        glm::vec2 x0 = v - i + glm::dot(i, glm::vec2(C.x, C.x));

        glm::vec2 i1 = (x0.x > x0.y) ? glm::vec2(1.0f, 0.0f) : glm::vec2(0.0f, 1.0f);
        glm::vec4 x12 = glm::vec4(x0.x, x0.y, x0.x, x0.y) + glm::vec4(C.x, C.x, C.z, C.z);
        x12.x -= i1.x;
        x12.y -= i1.y;

        i = Mod(i, 289.0f);
        glm::vec3 p = Permute(Permute(glm::vec3(i.y + 0.0f, i.y + i1.y, i.y + 1.0f)) +
            glm::vec3(i.x + 0.0f, i.x + i1.x, i.x + 1.0f));

        glm::vec3 m = glm::max(0.5f - glm::vec3(
            glm::dot(x0, x0),
            glm::dot(glm::vec2(x12.x, x12.y), glm::vec2(x12.x, x12.y)),
            glm::dot(glm::vec2(x12.z, x12.w), glm::vec2(x12.z, x12.w))
        ), 0.0f);
        m = m * m;
        m = m * m;

        glm::vec3 x = 2.0f * Fract(p * C.w) - 1.0f;
        glm::vec3 h = glm::abs(x) - 0.5f;
        glm::vec3 ox = glm::floor(x + 0.5f);
        glm::vec3 a0 = x - ox;
        m *= 1.79284291400159f - 0.85373472095314f * (a0 * a0 + h * h);

        glm::vec3 g;
        g.x = a0.x * x0.x + h.x * x0.y;
        g.y = a0.y * x12.x + h.y * x12.y;
        g.z = a0.z * x12.z + h.z * x12.w;

        float result = 130.0f * glm::dot(m, g);
        return result;
    }

    // Exact port of noise.glsl's fbm
    float Fbm(glm::vec2 pos, int octaves, float frequency, float lacunarity, float gain) {
        float sum = 0.0f;
        float amplitude = 1.0f;
        float freq = frequency;
        for (int i = 0; i < octaves; i++) {
            sum += SimplexNoise2D(pos * freq) * amplitude;
            freq *= lacunarity;
            amplitude *= gain;
        }
        return sum;
    }

    // Exact port of noise.glsl's ridgedNoise
    float RidgedNoise(glm::vec2 pos, int octaves, float frequency, float lacunarity, float gain) {
        float sum = 0.0f;
        float amplitude = 0.5f;
        float freq = frequency;
        for (int i = 0; i < octaves; i++) {
            float n = std::abs(SimplexNoise2D(pos * freq));
            n = 1.0f - n;
            n = n * n;
            sum += n * amplitude;
            freq *= lacunarity;
            amplitude *= gain;
           // std::cout << "CPU octave " << i << ": sum=" << sum << " n=" << n << " amplitude=" << amplitude << " freq=" << freq << std::endl;
        }
        return sum;
    }

    // Exact port of biome.glsl's getAridity
    float GetAridity(float worldX, float worldZ, float seed) {
        float seedOffset = HashSeed(seed + 7000.0f);
        float raw = SimplexNoise2D(glm::vec2(worldX, worldZ) * 0.0015f + glm::vec2(seedOffset));
        return (raw + 1.0f) * 0.5f;
    }

    // Exact port of biome.glsl's getElevationTrigger
    float GetElevationTrigger(float worldX, float worldZ, float seed) {
        float seedOffset = HashSeed(seed);
        float raw = RidgedNoise(glm::vec2(worldX, worldZ) * 0.0008f + glm::vec2(seedOffset), 3, 1.0f, 2.0f, 0.5f);
        return glm::clamp((raw - 0.05f) / 0.9f, 0.0f, 1.0f);
    }

    // Exact port of biome.glsl's getLatitude
    float GetLatitude(float worldZ) {
        const float LATITUDE_BAND_SIZE = 2000.0f;
        return glm::clamp(std::abs(worldZ) / LATITUDE_BAND_SIZE, 0.0f, 1.0f);
    }

    static int DetermineClimateBiome(float latitude, float aridity) {
        const float EPSILON = 0.001f;
        if (latitude >= 0.6f - EPSILON && latitude <= 1.0f) return BIOME_TUNDRA;
        if (latitude >= 0.0f && latitude <= 0.5f + EPSILON && aridity >= 0.6f - EPSILON && aridity <= 1.0f) return BIOME_DESERT;
        if (aridity >= 0.0f && aridity <= 0.5f + EPSILON) return BIOME_LAKE;
        return BIOME_PLAINS;
    }

    int DetermineBiome(float worldX, float worldZ, float seed) {
        float elevationTrigger = GetElevationTrigger(worldX, worldZ, seed);
        const float EPSILON = 0.001f;
        if (elevationTrigger > 0.6f - EPSILON) return BIOME_MOUNTAINS;
        if (elevationTrigger > 0.35f - EPSILON) return BIOME_HILLS;

        float latitude = GetLatitude(worldZ);
        float aridity = GetAridity(worldX, worldZ, seed);
        return DetermineClimateBiome(latitude, aridity);
    }
    float GetBiomeHeightScale(int biomeId) {
        if (biomeId == BIOME_PLAINS)    return 2.5f;
        if (biomeId == BIOME_DESERT)    return 2.0f;
        if (biomeId == BIOME_TUNDRA)    return 3.0f;
        if (biomeId == BIOME_HILLS)     return 6.0f;
        if (biomeId == BIOME_MOUNTAINS) return 12.0f;
        if (biomeId == BIOME_LAKE)      return 1.0f;
        return 1.0f;
    }

    float SampleRawHeightForBiome(float worldX, float worldZ, int biomeId, float seed) {
        float heightScale = GetBiomeHeightScale(biomeId);

        if (biomeId == BIOME_HILLS || biomeId == BIOME_MOUNTAINS) {
            float seedOffset = HashSeed(seed + 3000.0f);
            float n = Fbm(glm::vec2(worldX, worldZ) * 0.012f + glm::vec2(seedOffset), 5, 1.0f, 2.0f, 0.5f);
            return n * heightScale;
        }
        if (biomeId == BIOME_LAKE) {
            float seedOffset = HashSeed(seed + 4000.0f);
            float n = SimplexNoise2D(glm::vec2(worldX, worldZ) * 0.01f + glm::vec2(seedOffset));
            return -heightScale + n * 0.3f;
        }
        float seedOffset = HashSeed(seed + 1000.0f);
        float n = Fbm(glm::vec2(worldX, worldZ) * 0.012f + glm::vec2(seedOffset), 4, 1.0f, 2.0f, 0.5f);
        return n * heightScale;
    }

    static float SmoothStepGpu(float edge0, float edge1, float x) {
        float t = glm::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    void SampleBiomeWeights(float worldX, float worldZ, float seed, float weights[6]) {
        const float sampleRadius = 20.0f;
        const int ringCount = 3;
        const int samplesPerRing = 12;

        for (int i = 0; i < 6; i++) weights[i] = 0.0f;
        weights[DetermineBiome(worldX, worldZ, seed)] += 1.0f;

        for (int r = 1; r <= ringCount; r++) {
            float ringRadius = sampleRadius * (static_cast<float>(r) / ringCount);
            float ringWeight = 1.0f - (static_cast<float>(r) / (ringCount + 1));

            for (int i = 0; i < samplesPerRing; i++) {
                float angle = (static_cast<float>(i) / samplesPerRing) * 6.28318530718f;
                float sx = worldX + std::cos(angle) * ringRadius;
                float sz = worldZ + std::sin(angle) * ringRadius;
                int b = DetermineBiome(sx, sz, seed);
                weights[b] += ringWeight;
            }
        }
    }

    float SampleHeight(float worldX, float worldZ, float seed) {
        float weights[6];
        SampleBiomeWeights(worldX, worldZ, seed, weights);

        float totalWeight = 0.0f;
        float climateHeight = 0.0f;
        for (int b = 0; b < 6; b++) {
            if (weights[b] <= 0.0f) continue;
            climateHeight += SampleRawHeightForBiome(worldX, worldZ, b, seed) * weights[b];
            totalWeight += weights[b];
        }
        climateHeight /= totalWeight;

        float elevationTrigger = GetElevationTrigger(worldX, worldZ, seed);
        float hillsWeight = SmoothStepGpu(0.25f, 0.45f, elevationTrigger) * (1.0f - SmoothStepGpu(0.55f, 0.75f, elevationTrigger));
        float mountainsWeight = SmoothStepGpu(0.45f, 0.65f, elevationTrigger);
        float baseWeight = glm::clamp(1.0f - std::max(hillsWeight, mountainsWeight), 0.0f, 1.0f);

        float totalElevWeight = baseWeight + hillsWeight + mountainsWeight;
        baseWeight /= totalElevWeight;
        hillsWeight /= totalElevWeight;
        mountainsWeight /= totalElevWeight;

        float hillsHeight = SampleRawHeightForBiome(worldX, worldZ, BIOME_HILLS, seed);
        float mountainsHeight = SampleRawHeightForBiome(worldX, worldZ, BIOME_MOUNTAINS, seed);

        return climateHeight * baseWeight + hillsHeight * hillsWeight + mountainsHeight * mountainsWeight;
    }
}