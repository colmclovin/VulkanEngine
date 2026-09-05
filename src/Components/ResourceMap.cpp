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
    regionNoise.SetFrequency(0.02f); // LOW frequency = large contiguous regions

    FastNoiseLite densityNoise;
    densityNoise.SetSeed(seed + 3000); // different seed offset, independent pattern
    densityNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    densityNoise.SetFrequency(0.3f); // HIGH frequency = fine-grained variation within a region

    for (int z = 0; z < gridDepth; z++) {
        for (int x = 0; x < gridWidth; x++) {
            float regionValue = regionNoise.GetNoise(static_cast<float>(x), static_cast<float>(z));
            RegionType region = DetermineRegion(regionValue);

            ResourceCell &cell = m_Cells[z * gridWidth + x];

            if (region == RegionType::IronDeposit || region == RegionType::CopperDeposit) {
                float density = densityNoise.GetNoise(static_cast<float>(x), static_cast<float>(z));
                // Even within the deposit region, only some tiles actually have ore —
                // gives texture/richness variation instead of a flat solid block of color.
                if (density > -0.2f) {
                    cell.resource = (region == RegionType::IronDeposit) ? ItemId::IronOre : ItemId::CopperOre;
                    cell.amount = 500.0f;
                }
            }
            // Forest/Plains: no ore; tree placement handled separately by WorldGenerator using the same region logic
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
    // Wide, non-overlapping ranges = large contiguous areas, not thin bands
    if (regionNoiseValue > 0.5f) return RegionType::IronDeposit;
    if (regionNoiseValue > 0.15f) return RegionType::CopperDeposit;
    if (regionNoiseValue > -0.2f) return RegionType::Forest;
    return RegionType::Plains;
}

RegionType ResourceMap::GetRegionAtWorldPos(float worldX, float worldZ, int seed) const {
    FastNoiseLite regionNoise;
    regionNoise.SetSeed(seed + 1000); // MUST match the seed offset used in Generate()
    regionNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    regionNoise.SetFrequency(0.02f);
    return DetermineRegion(regionNoise.GetNoise(worldX / m_CellSize, worldZ / m_CellSize));
}