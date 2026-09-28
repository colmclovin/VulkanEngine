#pragma once
#include "../Rendering/Mesh.h"
#include "ResourceMap.h"
#include <entt/entt.hpp>
#include <memory>
#include <vector>
#include "ChunkCoord.h"

struct Chunk {
    ChunkCoord coord;
    uint32_t terrainSlot = UINT32_MAX;   // NEW — replaces terrainEntity/terrainMesh entirely
    std::vector<entt::entity> treeEntities;
    bool isGenerated = false;
};
