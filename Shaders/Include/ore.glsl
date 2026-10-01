// Shaders/Include/ore.glsl
#ifndef ORE_GLSL
#define ORE_GLSL

// Ore type IDs — must match your OreDatabase's ItemId order for whichever ores you register
const int ORE_NONE = -1;
const int ORE_COPPER = 0;
const int ORE_IRON = 1;
const int ORE_COAL = 2;
const int ORE_URANIUM = 3;

struct OreInfo {
    int oreType;
    float amount;
};

// Simple hash-based RNG, deterministic per (cellX, cellZ, seed) — mirrors std::mt19937 usage conceptually,
// though NOT bit-identical to it (a real hash function, not the same PRNG algorithm)
float hashToFloat(vec2 cell, float seed) {
    float seedOffset = hashSeed(seed);   // CHANGED — bound the seed before combining with cell coords
    float h = dot(cell, vec2(12.9898, 78.233)) + seedOffset;
    return fract(sin(h) * 43758.5453);
}

OreInfo getOreDepositAt(float worldX, float worldZ, float seed) {
    float distanceFromOrigin = length(vec2(worldX, worldZ));
    float distanceFactor = clamp(distanceFromOrigin / 2000.0, 0.0, 1.0);

    float cellSize = mix(500.0, 1000.0, distanceFactor);
    float depositRadius = mix(30.0, 80.0, distanceFactor);

    float cellX = floor(worldX / cellSize);
    float cellZ = floor(worldZ / cellSize);
    vec2 cell = vec2(cellX, cellZ);

    float jitterX = mix(0.2, 0.8, hashToFloat(cell + vec2(1.0, 0.0), seed));
    float jitterZ = mix(0.2, 0.8, hashToFloat(cell + vec2(0.0, 1.0), seed));

    float centerX = (cellX + jitterX) * cellSize;
    float centerZ = (cellZ + jitterZ) * cellSize;

    float dist = length(vec2(worldX - centerX, worldZ - centerZ));

    OreInfo result;
    result.oreType = ORE_NONE;
    result.amount = 0.0;

    if (dist > depositRadius) return result;

    // Ore selection — MUST match OreDatabase's rarity table and minDistanceFromOrigin exactly
    // (hardcoded here to mirror your current C++ OreDatabase::Init() values — update both together if they change)
    float totalRarity = 0.0;
    float rarities[4] = float[](0.35, 0.30, 0.25, 0.05);          // Copper, Iron, Coal, Uranium
    float minDistances[4] = float[](0.0, 0.0, 0.0, 800.0);
    float baseAmounts[4] = float[](200.0, 200.0, 150.0, 400.0);
    float amountPerDist[4] = float[](0.05, 0.05, 0.04, 0.2);

    for (int i = 0; i < 4; i++) {
        if (distanceFromOrigin < minDistances[i]) continue;
        totalRarity += rarities[i];
    }
    if (totalRarity <= 0.0) return result;

    float roll = hashToFloat(cell + vec2(2.0, 0.0), seed) * totalRarity;
    float cumulative = 0.0;
    for (int i = 0; i < 4; i++) {
        if (distanceFromOrigin < minDistances[i]) continue;
        cumulative += rarities[i];
        if (roll <= cumulative) {
            result.oreType = i;
            result.amount = baseAmounts[i] + amountPerDist[i] * distanceFromOrigin;
            return result;
        }
    }
    return result;
}

vec3 getOreColor(int oreType) {
    if (oreType == ORE_COPPER)   return vec3(0.85, 0.45, 0.25);
    if (oreType == ORE_IRON)     return vec3(0.55, 0.55, 0.65);
    if (oreType == ORE_COAL)     return vec3(0.15, 0.15, 0.15);
    if (oreType == ORE_URANIUM)  return vec3(0.4, 0.9, 0.3);
    return vec3(1.0);
}

#endif