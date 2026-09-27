// TerrainGenerator.h
#pragma once
#include "../Components/Mesh.h"
#include "ResourceMap.h"
#include <memory>
#include "GameSettings.h"
#include "../Game/BiomeDatabase.h"
#include "../Game/BiomeMap.h"
#include "../Components/DepletionMap.h"

class TerrainGenerator {
public:
    static std::shared_ptr<Mesh> GenerateHeightmapTerrain(const TerrainSettings &settings, ResourceMap& resourceMap);
    static float SampleHeight(float worldX, float worldZ, const TerrainSettings& settings);
    static float SampleNoise(float x, float z, float noiseScale, int seed);
    static float SampleRawHeightForBiome(float worldX, float worldZ, BiomeId biome, const TerrainSettings &settings);
    static glm::vec3 SampleBiomeColor(float worldX, float worldZ, const TerrainSettings &settings);
    static std::unordered_map<BiomeId, float> SampleBiomeWeights(float worldX, float worldZ, const TerrainSettings &settings);
    struct TerrainSample {
        float height;
        glm::vec3 color;
    };
    static TerrainSample SampleTerrain(float worldX, float worldZ, const TerrainSettings &settings, const DepletionMap &depletionMap);

private:
    static float SmoothStep(float edge0, float edge1, float x);
    static glm::vec3 ColorForResource(ItemId resource);

};