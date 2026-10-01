// OreDepositMap.cpp
#include "OreDepositMap.h"
#include <cmath>
#include <array>
#include "../Utils/GpuNoiseMatch.h"

float OreDepositMap::Fract(float x) {
    return x - std::floor(x);
}

float OreDepositMap::HashToFloat(float cellX, float cellZ, float seed, float saltX, float saltZ) {
    float seedOffset = GpuNoiseMatch::HashSeed(seed);   // CHANGED — reuse the shared, fixed HashSeed
    float h = (cellX + saltX) * 12.9898f + (cellZ + saltZ) * 78.233f + seedOffset;
    return Fract(std::sin(h) * 43758.5453f);
}

std::optional<OreDepositInfo> OreDepositMap::GetDepositAt(float worldX, float worldZ, int seed) {
    float distanceFromOrigin = std::sqrt(worldX * worldX + worldZ * worldZ);
    float distanceFactor = std::clamp(distanceFromOrigin / 2000.0f, 0.0f, 1.0f);

    float cellSize = glm::mix(500.0f, 1000.0f, distanceFactor);
    float depositRadius = glm::mix(30.0f, 80.0f, distanceFactor);

    float cellX = std::floor(worldX / cellSize);
    float cellZ = std::floor(worldZ / cellSize);

    // Matches ore.glsl: hashToFloat(cell + vec2(1.0, 0.0), seed), hashToFloat(cell + vec2(0.0, 1.0), seed)
    float jitterX = glm::mix(0.2f, 0.8f, HashToFloat(cellX, cellZ, static_cast<float>(seed), 1.0f, 0.0f));
    float jitterZ = glm::mix(0.2f, 0.8f, HashToFloat(cellX, cellZ, static_cast<float>(seed), 0.0f, 1.0f));

    float centerX = (cellX + jitterX) * cellSize;
    float centerZ = (cellZ + jitterZ) * cellSize;

    float dist = std::sqrt((worldX - centerX) * (worldX - centerX) + (worldZ - centerZ) * (worldZ - centerZ));
    if (dist > depositRadius) {
        return std::nullopt;
    }

    // Ore selection table — MUST match ore.glsl's hardcoded arrays exactly
    struct OreTableEntry { OreType type; ItemId item; float rarity; float minDistance; float baseAmount; float amountPerDistance; };
    static const std::array<OreTableEntry, 4> oreTable = { {
        { OreType::Copper,  ItemId::CopperOre,  0.35f, 0.0f,   200.0f, 0.05f },
        { OreType::Iron,    ItemId::IronOre,    0.30f, 0.0f,   200.0f, 0.05f },
        { OreType::Coal,    ItemId::Coal,       0.25f, 0.0f,   150.0f, 0.04f },
        { OreType::Uranium, ItemId::UraniumOre, 0.05f, 800.0f, 400.0f, 0.2f  },
    } };

    float totalRarity = 0.0f;
    for (auto& entry : oreTable) {
        if (distanceFromOrigin < entry.minDistance) continue;
        totalRarity += entry.rarity;
    }
    if (totalRarity <= 0.0f) return std::nullopt;

    // Matches ore.glsl: hashToFloat(cell + vec2(2.0, 0.0), seed) * totalRarity
    float roll = HashToFloat(cellX, cellZ, static_cast<float>(seed), 2.0f, 0.0f) * totalRarity;
    float cumulative = 0.0f;
    for (auto& entry : oreTable) {
        if (distanceFromOrigin < entry.minDistance) continue;
        cumulative += entry.rarity;
        if (roll <= cumulative) {
            float amount = entry.baseAmount + entry.amountPerDistance * distanceFromOrigin;
            return OreDepositInfo{ entry.item, amount };
        }
    }
    return std::nullopt;
}