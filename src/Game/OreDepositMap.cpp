// OreDepositMap.cpp
#include "OreDepositMap.h"
#include <FastNoiseLite.h>
#include <cmath>
#include <random>
#include <iostream>
// OreDepositMap.cpp — GetDepositAt, revised
std::optional<OreDepositInfo> OreDepositMap::GetDepositAt(float worldX, float worldZ, int seed) {
    float distanceFromOrigin = std::sqrt(worldX * worldX + worldZ * worldZ);
    float distanceFactor = glm::clamp(distanceFromOrigin / 2000.0f, 0.0f, 1.0f);

    float cellSize = glm::mix(500.0f, 1000.0f, distanceFactor);
    float depositRadius = glm::mix(30.0f, 80.0f, distanceFactor);

    int cellX = static_cast<int>(std::floor(worldX / cellSize));
    int cellZ = static_cast<int>(std::floor(worldZ / cellSize));

    std::mt19937 cellRng(static_cast<uint32_t>(cellX) * 73856093u ^ static_cast<uint32_t>(cellZ) * 19349663u ^ seed);
    std::uniform_real_distribution<float> jitterDist(0.2f, 0.8f);

    float centerX = (cellX + jitterDist(cellRng)) * cellSize;
    float centerZ = (cellZ + jitterDist(cellRng)) * cellSize;

    float dist = std::sqrt((worldX - centerX) * (worldX - centerX) + (worldZ - centerZ) * (worldZ - centerZ));
    if (dist > depositRadius) {
        return std::nullopt;
    }

    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    const auto& ores = OreDatabase::GetAll();
    float totalRarity = 0.0f;
    for (auto& ore : ores) {
        if (distanceFromOrigin < ore.minDistanceFromOrigin) continue;
        totalRarity += ore.rarity;
    }
    if (totalRarity <= 0.0f) return std::nullopt;

    float roll = dist01(cellRng) * totalRarity;
    float cumulative = 0.0f;
    for (auto& ore : ores) {
        if (distanceFromOrigin < ore.minDistanceFromOrigin) continue;
        cumulative += ore.rarity;
        if (roll <= cumulative) {
            float amount = ore.baseAmount + ore.amountPerDistance * distanceFromOrigin;
            return OreDepositInfo{ ore.item, amount };
        }
    }
    return std::nullopt;
}