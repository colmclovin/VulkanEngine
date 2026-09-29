// Shaders/Include/biome.glsl
#ifndef BIOME_GLSL
#define BIOME_GLSL

#include "noise.glsl"

const int BIOME_PLAINS = 0;
const int BIOME_DESERT = 1;
const int BIOME_TUNDRA = 2;
const int BIOME_HILLS = 3;
const int BIOME_MOUNTAINS = 4;
const int BIOME_LAKE = 5;


float getElevationTrigger(float worldX, float worldZ, float seed) {
    float seedOffset = hashSeed(seed);
    float raw = ridgedNoise(vec2(worldX, worldZ) * 0.0008 + vec2(seedOffset), 3, 1.0, 2.0, 0.5);
    return clamp((raw - 0.05) / 0.9, 0.0, 1.0);
}

float getAridity(float worldX, float worldZ, float seed) {
    float seedOffset = hashSeed(seed + 7000.0);   // add a distinct constant before hashing, so aridity/elevation get different offsets from the same seed
    float raw = simplexNoise2D(vec2(worldX, worldZ) * 0.0015 + vec2(seedOffset));
    return (raw + 1.0) * 0.5;
}

float getLatitude(float worldZ) {   // CHANGED — worldExtentZ parameter removed
    const float LATITUDE_BAND_SIZE = 2000.0;
    return clamp(abs(worldZ) / LATITUDE_BAND_SIZE, 0.0, 1.0);
}

int determineClimateBiome(float latitude, float aridity) {
    // Mirrors BiomeDatabase's evaluation order: Tundra, Desert, Lake, Plains (fallback)
    if (latitude >= 0.6 && latitude <= 1.0) {
        return BIOME_TUNDRA;
    }
    if (latitude >= 0.0 && latitude <= 0.5 && aridity >= 0.6 && aridity <= 1.0) {
        return BIOME_DESERT;
    }
    if (aridity >= 0.0 && aridity <= 0.5) {
        return BIOME_LAKE;
    }
    return BIOME_PLAINS;
}

int determineBiome(float worldX, float worldZ, float seed) {   // CHANGED — worldExtentZ removed
    float elevationTrigger = getElevationTrigger(worldX, worldZ, seed);

    if (elevationTrigger > 0.6) {
        return BIOME_MOUNTAINS;
    }
    if (elevationTrigger > 0.35) {
        return BIOME_HILLS;
    }

    float latitude = getLatitude(worldZ);   // CHANGED
    float aridity = getAridity(worldX, worldZ, seed);
    return determineClimateBiome(latitude, aridity);
}

#endif