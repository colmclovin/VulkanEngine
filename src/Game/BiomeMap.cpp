// BiomeMap.cpp
#include "BiomeMap.h"
#include <FastNoiseLite.h>
#include <cmath>

float BiomeMap::GetLatitude(float worldZ, float worldExtentZ) {
    float center = worldExtentZ * 0.5f; // treat the middle of the world as the equator
    float halfExtent = worldExtentZ * 0.5f;
    return glm::clamp(std::abs(worldZ - center) / halfExtent, 0.0f, 1.0f);
}

float BiomeMap::GetAridity(float worldX, float worldZ, int seed) {
    FastNoiseLite noise;
    noise.SetSeed(seed + 7000); // distinct offset from height/resource noise
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(0.15f); // low frequency — large, slow-varying dry/wet regions
    float raw = noise.GetNoise(worldX, worldZ); // [-1, 1]
    return (raw + 1.0f) * 0.5f; // remap to [0, 1]
}

float BiomeMap::GetElevationTrigger(float worldX, float worldZ, int seed) {
    FastNoiseLite noise;
    noise.SetSeed(seed + 9000);
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFractalType(FastNoiseLite::FractalType_Ridged);
    noise.SetFractalOctaves(3);
    noise.SetFrequency(0.0008f);
    float raw = noise.GetNoise(worldX, worldZ);

    // Empirically, this ridged config outputs roughly [-0.5, 0.6] — remap to a clean [0,1]
    return glm::clamp((raw + 0.5f) / 1.1f, 0.0f, 1.0f);
}

BiomeId BiomeMap::GetBiomeAt(float worldX, float worldZ, float worldExtentZ, int seed) {
    float elevationTrigger = GetElevationTrigger(worldX, worldZ, seed);

    if (elevationTrigger > 0.6f) return BiomeId::Mountains;
    if (elevationTrigger > 0.35f) return BiomeId::Hills;

    // Otherwise, fall through to normal climate-based biome
    float latitude = GetLatitude(worldZ, worldExtentZ);
    float aridity = GetAridity(worldX, worldZ, seed);
    return BiomeDatabase::DetermineBiome(latitude, aridity);
}

BiomeId BiomeMap::GetClimateBiomeAt(float worldX, float worldZ, float worldExtentZ, int seed) {
    float latitude = GetLatitude(worldZ, worldExtentZ);
    float aridity = GetAridity(worldX, worldZ, seed);
    return BiomeDatabase::DetermineBiome(latitude, aridity);
}