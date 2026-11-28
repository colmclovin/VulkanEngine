#include "WorldGenerator.h"
#include "../Utils/GpuNoiseMatch.h"

glm::vec3 WorldGenerator::FindSpawnPoint(const TerrainSettings &terrainSettings, int seed, std::mt19937 &rng) {
    float worldWidth = terrainSettings.gridWidth * terrainSettings.cellSize;
    float worldDepth = terrainSettings.gridDepth * terrainSettings.cellSize;
    std::uniform_real_distribution<float> xDist(0.0f, worldWidth);
    std::uniform_real_distribution<float> zDist(0.0f, worldDepth);

    // Use the GPU-matching terrain functions so the spawn point agrees with the rendered terrain
    const float fseed = static_cast<float>(seed);
    const int maxAttempts = 200;
    for (int i = 0; i < maxAttempts; i++) {
        float x = xDist(rng);
        float z = zDist(rng);
        int biome = GpuNoiseMatch::DetermineBiome(x, z, fseed);
        if (biome == BIOME_LAKE || biome == BIOME_MOUNTAINS) continue;
        return glm::vec3(x, GpuNoiseMatch::SampleSurfaceHeight(x, z, fseed), z);
    }

    float cx = worldWidth * 0.5f, cz = worldDepth * 0.5f;
    return glm::vec3(cx, GpuNoiseMatch::SampleSurfaceHeight(cx, cz, fseed), cz);
}
