#pragma once
#include <glm/glm.hpp>
#include <random>
#include "../Utils/GameSettings.h"

class WorldGenerator {
public:
    // Random point inside the settings' grid area that isn't lake or mountain, at terrain surface height.
    static glm::vec3 FindSpawnPoint(const TerrainSettings &terrainSettings, int seed, std::mt19937 &rng);
};
