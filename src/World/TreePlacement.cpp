// TreePlacement.cpp
#include "TreePlacement.h"
#include "../Game/BiomeMap.h"
#include "../Game/OreDepositMap.h"
#include "TerrainGenerator.h"
#include "../Utils/GameSettings.h"
#include "../Utils/GpuNoiseMatch.h"
#include <cmath>

float TreePlacement::HashJitter(glm::vec2 pos, float salt) {
    glm::vec2 wrapped = glm::mod(pos, 1000.0f);   // MUST match GLSL exactly
    float h = glm::dot(wrapped, glm::vec2(12.9898f, 78.233f)) + salt;
    float s = std::sin(h) * 43758.5453f;
    return s - std::floor(s);
}

std::vector<TreeCandidate> TreePlacement::GetTreesInChunk(int chunkCoordX, int chunkCoordZ, float chunkWorldSize, const TerrainSettings& settings) {
    std::vector<TreeCandidate> results;

    float chunkOriginX = chunkCoordX * chunkWorldSize;
    float chunkOriginZ = chunkCoordZ * chunkWorldSize;
    const int samplesPerAxis = 4;
    const float sampleSpacing = 8.0f;

    for (int sz = 0; sz < samplesPerAxis; sz++) {
        for (int sx = 0; sx < samplesPerAxis; sx++) {
            float worldX = chunkOriginX + sx * sampleSpacing;
            float worldZ = chunkOriginZ + sz * sampleSpacing;

            int biomeId = GpuNoiseMatch::DetermineBiome(worldX, worldZ, static_cast<float>(settings.seed));
            float trigger = GpuNoiseMatch::GetElevationTrigger(-32.0f, 312.0f, (float)settings.seed);
            std::cout << "CPU elevationTrigger at (-32,312) = " << trigger << std::endl;
            if (biomeId == BIOME_LAKE) continue;

            float treeDensity = 0.0f;
            if (biomeId == BIOME_PLAINS) treeDensity = 0.2f;
            else if (biomeId == BIOME_HILLS) treeDensity = 0.25f;
            else if (biomeId == BIOME_TUNDRA) treeDensity = 0.05f;
            if (treeDensity <= 0.0f) continue;

            auto ore = OreDepositMap::GetDepositAt(worldX, worldZ, settings.seed);
            if (ore.has_value()) continue;

            float aridity = GpuNoiseMatch::GetAridity(worldX, worldZ, static_cast<float>(settings.seed));   // CHANGED
            float effectiveDensity = treeDensity * (1.0f - aridity);

            float roll = HashJitter(glm::vec2(worldX, worldZ), 1.0f);
            float threshold = 1.0f - (effectiveDensity * 2.0f);
            const float EPSILON = 0.001f;
            if (roll < threshold - EPSILON) continue;

            float jitterX = (HashJitter(glm::vec2(worldX, worldZ), 2.0f) - 0.5f) * sampleSpacing;
            float jitterZ = (HashJitter(glm::vec2(worldX, worldZ), 3.0f) - 0.5f) * sampleSpacing;
            float finalX = worldX + jitterX;
            float finalZ = worldZ + jitterZ;

            float height = GpuNoiseMatch::SampleHeight(finalX, finalZ, static_cast<float>(settings.seed));
            results.push_back({ glm::vec3(finalX, height, finalZ) });
        }
    }
    return results;
}