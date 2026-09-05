// TerrainRaycast.h
#pragma once
#include "GameSettings.h"
#include <glm/glm.hpp>

class TerrainRaycast {
public:
    static glm::vec3 RaycastToTerrain(glm::vec3 rayOrigin, glm::vec3 rayDir, const TerrainSettings &terrainSettings);
};