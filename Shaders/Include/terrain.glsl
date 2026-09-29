// Shaders/Include/terrain.glsl
#ifndef TERRAIN_GLSL
#define TERRAIN_GLSL

#include "biome.glsl"

vec3 getBiomeColor(int biomeId) {
    if (biomeId == BIOME_PLAINS)    return vec3(0.35, 0.6, 0.25);
    if (biomeId == BIOME_DESERT)    return vec3(0.85, 0.75, 0.45);
    if (biomeId == BIOME_TUNDRA)    return vec3(0.85, 0.87, 0.9);
    if (biomeId == BIOME_HILLS)     return vec3(0.4, 0.55, 0.3);
    if (biomeId == BIOME_MOUNTAINS) return vec3(0.5, 0.48, 0.45);
    if (biomeId == BIOME_LAKE)      return vec3(0.2, 0.3, 0.5);
    return vec3(1.0, 0.0, 1.0);
}

float getBiomeHeightScale(int biomeId) {
    if (biomeId == BIOME_PLAINS)    return 2.5;
    if (biomeId == BIOME_DESERT)    return 2.0;
    if (biomeId == BIOME_TUNDRA)    return 3.0;
    if (biomeId == BIOME_HILLS)     return 15.0;
    if (biomeId == BIOME_MOUNTAINS) return 35.0;
    if (biomeId == BIOME_LAKE)      return 3.0;
    return 1.0;
}

// heightStyle: 0=Gentle (Plains/Desert/Tundra), 1=Rugged (Hills/Mountains), 2=Carved (Lake)
float sampleRawHeightForBiome(float worldX, float worldZ, int biomeId, float seed) {
    float heightScale = getBiomeHeightScale(biomeId);

    if (biomeId == BIOME_HILLS || biomeId == BIOME_MOUNTAINS) {
        float n = fbm(vec2(worldX, worldZ) * 0.006 + vec2(seed * 1000.0), 5, 1.0, 2.0, 0.5);   // CHANGED — 3 to 5 octaves
        return n * heightScale;
    }
    if (biomeId == BIOME_LAKE) {
        float n = simplexNoise2D(vec2(worldX, worldZ) * 0.01 + vec2(seed * 1000.0));
        return -heightScale + n * 0.3;
    }
    float n = fbm(vec2(worldX, worldZ) * 0.004 + vec2(seed * 1000.0), 4, 1.0, 2.0, 0.5);   // CHANGED — 2 to 4 octaves
    return n * heightScale;
}

float smoothStepGLSL(float edge0, float edge1, float x) {
    float t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

void sampleBiomeWeights(float worldX, float worldZ, float seed, out float weights[6]) {
    const float sampleRadius = 20.0;
    const int ringCount = 3;
    const int samplesPerRing = 12;

    for (int i = 0; i < 6; i++) weights[i] = 0.0;

    weights[determineBiome(worldX, worldZ, seed)] += 1.0;

    for (int r = 1; r <= ringCount; r++) {
        float ringRadius = sampleRadius * (float(r) / float(ringCount));
        float ringWeight = 1.0 - (float(r) / float(ringCount + 1));

        for (int i = 0; i < samplesPerRing; i++) {
            float angle = (float(i) / float(samplesPerRing)) * 6.28318530718;   // 2*PI
            float sx = worldX + cos(angle) * ringRadius;
            float sz = worldZ + sin(angle) * ringRadius;
            int b = determineBiome(sx, sz, seed);
            weights[b] += ringWeight;
        }
    }
}

struct TerrainSample {
    float height;
    vec3 color;
};

TerrainSample sampleTerrain(float worldX, float worldZ, float seed) {
    // --- Climate biome, ring-blended ---
    float weights[6];
    sampleBiomeWeights(worldX, worldZ, seed, weights);

    float totalWeight = 0.0;
    float climateHeight = 0.0;
    vec3 climateColor = vec3(0.0);
    for (int b = 0; b < 6; b++) {
        if (weights[b] <= 0.0) continue;
        climateHeight += sampleRawHeightForBiome(worldX, worldZ, b, seed) * weights[b];
        climateColor += getBiomeColor(b) * weights[b];
        totalWeight += weights[b];
    }
    climateHeight /= totalWeight;
    climateColor /= totalWeight;

    // --- Elevation trigger, smooth three-way blend ---
    float elevationTrigger = getElevationTrigger(worldX, worldZ, seed);

    float hillsWeight = smoothStepGLSL(0.25, 0.45, elevationTrigger) * (1.0 - smoothStepGLSL(0.55, 0.75, elevationTrigger));
    float mountainsWeight = smoothStepGLSL(0.45, 0.65, elevationTrigger);
    float baseWeight = clamp(1.0 - max(hillsWeight, mountainsWeight), 0.0, 1.0);

    float totalElevWeight = baseWeight + hillsWeight + mountainsWeight;
    baseWeight /= totalElevWeight;
    hillsWeight /= totalElevWeight;
    mountainsWeight /= totalElevWeight;

    vec3 hillsColor = getBiomeColor(BIOME_HILLS);
    vec3 mountainsColor = getBiomeColor(BIOME_MOUNTAINS);

    float hillsHeight = sampleRawHeightForBiome(worldX, worldZ, BIOME_HILLS, seed);
    float mountainsHeight = sampleRawHeightForBiome(worldX, worldZ, BIOME_MOUNTAINS, seed);

    float finalHeight = climateHeight * baseWeight + hillsHeight * hillsWeight + mountainsHeight * mountainsWeight;

    vec3 finalColor = climateColor * baseWeight
                     + mix(climateColor, hillsColor, 0.5) * hillsWeight
                     + mix(climateColor, mountainsColor, 0.7) * mountainsWeight;

    TerrainSample result;
    result.height = finalHeight;
    result.color = finalColor;
    return result;
}


#endif