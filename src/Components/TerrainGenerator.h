// TerrainGenerator.h
#pragma once
#include "../Components/Mesh.h"
#include <memory>
#include "GameSettings.h"
class TerrainGenerator {
public:
    static std::shared_ptr<Mesh> GenerateHeightmapTerrain(
        int gridWidth, int gridDepth,      // number of vertices per side
        float cellSize,                     // world-space distance between vertices
        float heightScale,                  // max height displacement
        float noiseScale = 0.1f,            // controls hill frequency — smaller = broader hills
        int seed = 1337);
    static float SampleHeight(float worldX, float worldZ, const TerrainSettings& settings);
private:
    static float SampleNoise(float x, float z, float noiseScale, int seed);
};