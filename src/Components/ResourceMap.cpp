#include "ResourceMap.h"
#include <FastNoiseLite.h>



void ResourceMap::Generate(int gridWidth, int gridDepth, float cellSize, int seed) {
    m_GridWidth = gridWidth;
    m_GridDepth = gridDepth;
    m_CellSize = cellSize;
    m_Cells.assign(static_cast<size_t>(gridWidth) * gridDepth, ResourceCell{});

    FastNoiseLite regionNoise;
    regionNoise.SetSeed(seed + 1000);
    regionNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    regionNoise.SetFrequency(0.02f);

    FastNoiseLite densityNoise;
    densityNoise.SetSeed(seed + 3000);
    densityNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    densityNoise.SetFrequency(0.3f);

    for (int z = 0; z < gridDepth; z++) {
        for (int x = 0; x < gridWidth; x++) {
            glm::vec2 warped = DomainWarp(static_cast<float>(x), static_cast<float>(z), seed); // CHANGED
            float regionValue = regionNoise.GetNoise(warped.x, warped.y); // CHANGED — sample warped coords
            RegionType region = DetermineRegion(regionValue);

            ResourceCell &cell = m_Cells[z * gridWidth + x];

            if (region == RegionType::IronDeposit || region == RegionType::CopperDeposit || region == RegionType::CoalDeposit) {
                float density = densityNoise.GetNoise(static_cast<float>(x), static_cast<float>(z)); // density stays unwarped — fine detail within a region
                if (density > -0.2f) {
                    cell.resource = (region == RegionType::IronDeposit) ? ItemId::IronOre : (region == RegionType::CopperDeposit) ? ItemId::CopperOre :
                                                                                                                                    ItemId::Coal;
                    cell.amount = 500.0f;
                }
            }
        }
    }
}

ResourceCell* ResourceMap::GetCellAtWorldPos(float worldX, float worldZ) {
    int x = static_cast<int>(std::round(worldX / m_CellSize));
    int z = static_cast<int>(std::round(worldZ / m_CellSize));
    if (x < 0 || x >= m_GridWidth || z < 0 || z >= m_GridDepth) return nullptr;
    return &m_Cells[z * m_GridWidth + x];
}

void ResourceMap::ExtractFromCell(ResourceCell* cell, float amount) {
    if (!cell || cell->resource == ItemId::None) return;
    cell->amount -= amount;
    if (cell->amount <= 0.0f) {
        cell->amount = 0.0f;
        cell->resource = ItemId::None;
    }
}


RegionType ResourceMap::DetermineRegion(float regionNoiseValue) {
    if (regionNoiseValue > 0.6f) return RegionType::IronDeposit;
    if (regionNoiseValue > 0.35f) return RegionType::CopperDeposit;
    if (regionNoiseValue > 0.15f) return RegionType::CoalDeposit;
    if (regionNoiseValue > -0.2f) return RegionType::Forest;
    return RegionType::Plains;
}

RegionType ResourceMap::GetRegionAtWorldPos(float worldX, float worldZ, int seed) const {
    glm::vec2 warped = DomainWarp(worldX / m_CellSize, worldZ / m_CellSize, seed); // CHANGED

    FastNoiseLite regionNoise;
    regionNoise.SetSeed(seed + 1000);
    regionNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    regionNoise.SetFrequency(0.02f);
    return DetermineRegion(regionNoise.GetNoise(warped.x, warped.y));
}
glm::vec2 ResourceMap::DomainWarp(float x, float z, int seed) {
    FastNoiseLite warpNoiseX;
    warpNoiseX.SetSeed(seed + 5000);
    warpNoiseX.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    warpNoiseX.SetFrequency(0.01f); // low frequency — large, slow-moving distortion

    FastNoiseLite warpNoiseZ;
    warpNoiseZ.SetSeed(seed + 6000);
    warpNoiseZ.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    warpNoiseZ.SetFrequency(0.01f);

    float warpStrength = 25.0f; // how far coordinates get pushed — tune to taste
    float warpedX = x + warpNoiseX.GetNoise(x, z) * warpStrength;
    float warpedZ = z + warpNoiseZ.GetNoise(x, z) * warpStrength;

    return glm::vec2(warpedX, warpedZ);
}