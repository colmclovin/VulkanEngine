#include "ResourceMap.h"
#include <FastNoiseLite.h>



void ResourceMap::Generate(int gridWidth, int gridDepth, float cellSize, int seed) {
m_GridWidth = gridWidth;
m_GridDepth = gridDepth;
m_CellSize = cellSize;
m_Cells.assign(static_cast<size_t>(gridWidth)* gridDepth, ResourceCell{});

FastNoiseLite noise;
noise.SetSeed(seed + 1000);
noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
noise.SetFrequency(0.08f);

for (int z = 0; z < gridDepth; z++) {
    for (int x = 0; x < gridWidth; x++) {
        float n = noise.GetNoise(static_cast<float>(x), static_cast<float>(z));
        ResourceCell& cell = m_Cells[z * gridWidth + x];

        if (n > 0.6f) {
            cell.resource = ItemId::IronOre;
            cell.amount = 500.0f;
        }
        else if (n > 0.4f) {
            cell.resource = ItemId::CopperOre;
            cell.amount = 500.0f;
        }
        else {
            cell.resource = ItemId::None;
            cell.amount = 0.0f;
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