#pragma once
#include <glm/glm.hpp>

namespace GpuNoiseMatch {
    float HashSeed(float seed);
    float SimplexNoise2D(glm::vec2 v);
    float Fbm(glm::vec2 pos, int octaves, float frequency, float lacunarity, float gain);
    float RidgedNoise(glm::vec2 pos, int octaves, float frequency, float lacunarity, float gain);

    float GetAridity(float worldX, float worldZ, float seed);
    float GetElevationTrigger(float worldX, float worldZ, float seed);
    float GetLatitude(float worldZ);
    int DetermineBiome(float worldX, float worldZ, float seed);   // returns BIOME_* int constant

    float GetBiomeHeightScale(int biomeId);
    float SampleRawHeightForBiome(float worldX, float worldZ, int biomeId, float seed);
    void SampleBiomeWeights(float worldX, float worldZ, float seed, float weights[6]);
    float SampleHeight(float worldX, float worldZ, float seed);   // the real height-matching function

}

// Match biome.glsl's constants exactly
constexpr int BIOME_PLAINS = 0;
constexpr int BIOME_DESERT = 1;
constexpr int BIOME_TUNDRA = 2;
constexpr int BIOME_HILLS = 3;
constexpr int BIOME_MOUNTAINS = 4;
constexpr int BIOME_LAKE = 5;