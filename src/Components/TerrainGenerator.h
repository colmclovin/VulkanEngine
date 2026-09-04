// TerrainGenerator.h
#pragma once
#include "../Components/Mesh.h"
#include "ResourceMap.h"
#include <memory>
#include "GameSettings.h"
class TerrainGenerator {
public:
    static std::shared_ptr<Mesh> GenerateHeightmapTerrain(
        int gridWidth, int gridDepth,      // number of vertices per side
        float cellSize,                     // world-space distance between vertices
        float heightScale,                  // max height displacement
        float noiseScale,            // controls hill frequency — smaller = broader hills
        int seed,
        ResourceMap& resourceMap);
    static float SampleHeight(float worldX, float worldZ, const TerrainSettings& settings);
    static float SampleNoise(float x, float z, float noiseScale, int seed);
private:
    static glm::vec3 ColorForResource(ItemId resource);
};