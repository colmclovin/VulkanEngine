// BiomeMap.cpp
#include "BiomeMap.h"
#include <FastNoiseLite.h>
#include <cmath>

float BiomeMap::GetLatitude(float worldZ) {
    const float LATITUDE_BAND_SIZE = 2000.0f;   // world units from equator (z=0) to full pole — tune to taste
    return glm::clamp(std::abs(worldZ) / LATITUDE_BAND_SIZE, 0.0f, 1.0f);
}

BiomeId BiomeMap::GetBiomeAt(float worldX, float worldZ, int seed) {
    float elevationTrigger = GetElevationTrigger(worldX, worldZ, seed);

    if (elevationTrigger > 0.6f) return BiomeId::Mountains;
    if (elevationTrigger > 0.35f) return BiomeId::Hills;

    float latitude = GetLatitude(worldZ);   // CHANGED — no worldExtentZ
    float aridity = GetAridity(worldX, worldZ, seed);
    return BiomeDatabase::DetermineBiome(latitude, aridity);
}

float BiomeMap::GetAridity(float worldX, float worldZ, int seed) {
    static thread_local FastNoiseLite noise;
    static thread_local int lastSeed = -1;

    if (lastSeed != seed) {
        noise.SetSeed(seed + 7000);
        noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        noise.SetFrequency(0.0015f);
        lastSeed = seed;
    }

    float raw = noise.GetNoise(worldX, worldZ);
    return (raw + 1.0f) * 0.5f;
}

float BiomeMap::GetElevationTrigger(float worldX, float worldZ, int seed) {
    static thread_local FastNoiseLite noise;
    static thread_local int lastSeed = -1;

    if (lastSeed != seed) {
        noise.SetSeed(seed + 9000);
        noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        noise.SetFractalType(FastNoiseLite::FractalType_Ridged);
        noise.SetFractalOctaves(3);
        noise.SetFrequency(0.0008f);
        lastSeed = seed;
    }

    float raw = noise.GetNoise(worldX, worldZ);
    return glm::clamp((raw + 0.5f) / 1.1f, 0.0f, 1.0f);
}



BiomeId BiomeMap::GetClimateBiomeAt(float worldX, float worldZ, float worldExtentZ, int seed) {
    float latitude = GetLatitude(worldZ);
    float aridity = GetAridity(worldX, worldZ, seed);
    return BiomeDatabase::DetermineBiome(latitude, aridity);
}