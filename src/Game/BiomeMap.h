// BiomeMap.h
#pragma once
#include "BiomeDatabase.h"
#include <glm/glm.hpp>

class BiomeMap {
public:
    static float GetLatitude(float worldZ, float worldExtentZ); // 0 at equator (z=0), 1 at either pole
    static float GetAridity(float worldX, float worldZ, int seed);
    // BiomeMap.h — add
    static float GetElevationTrigger(float worldX, float worldZ, int seed);
    static BiomeId GetBiomeAt(float worldX, float worldZ, float worldExtentZ, int seed); // updated to apply the trigger
    static BiomeId GetClimateBiomeAt(float worldX, float worldZ, float worldExtentZ, int seed); // climate only, ignoring elevation
};