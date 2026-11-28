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

const float EPSILON = 0.001;

// ---- Biome tuning. MUST match the same constants in src/Utils/GpuNoiseMatch.cpp ----
const float BIOME_SCALE = 3.0;               // multiplies the size of every biome (bigger = larger regions)
const float ELEVATION_FREQUENCY = 0.0008 / BIOME_SCALE; // hills/mountains noise
const float ARIDITY_FREQUENCY = 0.0015 / BIOME_SCALE;   // lake/plains/desert noise
const float LATITUDE_BAND_SIZE = 2000.0 * BIOME_SCALE;  // world units from the equator (z = 0) to full tundra
const float LAKE_MAX_ARIDITY = 0.2;          // aridity at or below this is lake (was 0.5: ~half of flat land)
const float DESERT_MIN_ARIDITY = 0.6;

float getElevationTrigger(float worldX, float worldZ, float seed) {
    float seedOffset = hashSeed(seed);
    float raw = ridgedNoise(vec2(worldX, worldZ) * ELEVATION_FREQUENCY + vec2(seedOffset), 3, 1.0, 2.0, 0.5);
    return clamp((raw - 0.05) / 0.9, 0.0, 1.0);
}

float getAridity(float worldX, float worldZ, float seed) {
    float seedOffset = hashSeed(seed + 7000.0);   // add a distinct constant before hashing, so aridity/elevation get different offsets from the same seed
    float raw = simplexNoise2D(vec2(worldX, worldZ) * ARIDITY_FREQUENCY + vec2(seedOffset));
    return (raw + 1.0) * 0.5;
}

float getLatitude(float worldZ) {
    return clamp(abs(worldZ) / LATITUDE_BAND_SIZE, 0.0, 1.0);
}
int determineClimateBiome(float latitude, float aridity) {
    if (latitude >= 0.6 && latitude <= 1.0) return BIOME_TUNDRA;
    if (latitude >= 0.0 && latitude <= 0.5 && aridity >= DESERT_MIN_ARIDITY && aridity <= 1.0) return BIOME_DESERT;
    if (aridity >= 0.0 && aridity <= LAKE_MAX_ARIDITY) return BIOME_LAKE;
    return BIOME_PLAINS;
}

int determineBiome(float worldX, float worldZ, float seed) {
    float elevationTrigger = getElevationTrigger(worldX, worldZ, seed);
    if (elevationTrigger > 0.6) return BIOME_MOUNTAINS;
    if (elevationTrigger > 0.35) return BIOME_HILLS;
    float latitude = getLatitude(worldZ);
    float aridity = getAridity(worldX, worldZ, seed);
    return determineClimateBiome(latitude, aridity);
}



#endif