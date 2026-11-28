#pragma once
#include <cstdint>
#include "ChunkCoord.h"

struct Chunk {
    ChunkCoord coord;
    uint32_t terrainSlot = UINT32_MAX; // slot in TerrainRenderer's shared vertex/index buffers
    uint32_t treeSlot = UINT32_MAX;    // slot in TreeRenderer's instance buffer
    bool isGenerated = false;          // false until the GPU generation has been recorded
};
