// TerrainRaycast.cpp
#include "TerrainRaycast.h"
#include "TerrainGenerator.h"

glm::vec3 TerrainRaycast::RaycastToTerrain(glm::vec3 rayOrigin, glm::vec3 rayDir, const TerrainSettings &terrainSettings) {
    const float stepSize = 0.25f;
    const float maxDistance = 200.0f;

    glm::vec3 pos = rayOrigin;
    for (float t = 0.0f; t < maxDistance; t += stepSize) {
        pos = rayOrigin + rayDir * t;
        float terrainY = TerrainGenerator::SampleHeight(pos.x, pos.z, terrainSettings);
        if (pos.y <= terrainY) {
            return glm::vec3(pos.x, terrainY, pos.z);
        }
    }
    return rayOrigin + rayDir * maxDistance;
}