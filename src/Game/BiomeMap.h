// BiomeMap.h
#pragma once
#include "BiomeDatabase.h"
#include <glm/glm.hpp>

class BiomeMap {
public:
    static float GetLatitude(float worldZ);   // CHANGED — worldExtentZ parameter removed
    static BiomeId GetBiomeAt(float worldX, float worldZ, int seed);
    static float GetAridity(float worldX, float worldZ, int seed);
    // BiomeMap.h — add
    static float GetElevationTrigger(float worldX, float worldZ, int seed);
    static BiomeId GetClimateBiomeAt(float worldX, float worldZ, float worldExtentZ, int seed); // climate only, ignoring elevation
};