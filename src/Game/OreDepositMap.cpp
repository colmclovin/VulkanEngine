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

    float cellSize = glm::mix(30.0f, 1000.0f, distanceFactor); // world units per cell — directly controls spacing, no noise-frequency guesswork
    float depositRadius = glm::mix(8.0f, 30.0f, distanceFactor); // actual radius in world units — directly controls patch size, no threshold guesswork

    // Which grid cell are we in?
    int cellX = static_cast<int>(std::floor(worldX / cellSize));
    int cellZ = static_cast<int>(std::floor(worldZ / cellSize));

    // Deterministically jitter this cell's deposit center within the cell, using a hash of its coordinates
    std::mt19937 cellRng(static_cast<uint32_t>(cellX) * 73856093u ^ static_cast<uint32_t>(cellZ) * 19349663u ^ seed);
    std::uniform_real_distribution<float> jitterDist(0.2f, 0.8f); // keep the center away from cell edges

    float centerX = (cellX + jitterDist(cellRng)) * cellSize;
    float centerZ = (cellZ + jitterDist(cellRng)) * cellSize;

    float dist = std::sqrt((worldX - centerX) * (worldX - centerX) + (worldZ - centerZ) * (worldZ - centerZ));
    if (dist > depositRadius) {
        return std::nullopt; // outside this cell's deposit blob
    }

    // Inside the blob — pick which ore, using the SAME rng state (already seeded per-cell, deterministic)
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    const auto &ores = OreDatabase::GetAll();
    float totalRarity = 0.0f;
    for (auto &ore : ores) {
        if (distanceFromOrigin < ore.minDistanceFromOrigin) continue;
        totalRarity += ore.rarity;
    }
    if (totalRarity <= 0.0f) return std::nullopt;

    float roll = dist01(cellRng) * totalRarity;
    float cumulative = 0.0f;
    for (auto &ore : ores) {
        if (distanceFromOrigin < ore.minDistanceFromOrigin) continue;
        cumulative += ore.rarity;
        if (roll <= cumulative) {
            float amount = ore.baseAmount + ore.amountPerDistance * distanceFromOrigin;
            return OreDepositInfo{ ore.item, amount };
        }
    }
    return std::nullopt;
}