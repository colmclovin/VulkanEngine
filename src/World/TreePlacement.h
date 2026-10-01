// TreePlacement.h
#pragma once
#include <vector>
#include <glm/glm.hpp>

struct TerrainSettings;

struct TreeCandidate {
    glm::vec3 position;
};

class TreePlacement {
public:
    static std::vector<TreeCandidate> GetTreesInChunk(int chunkCoordX, int chunkCoordZ, float chunkWorldSize, const TerrainSettings& settings);

private:
    static float HashJitter(glm::vec2 pos, float salt);
};