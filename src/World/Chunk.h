#pragma once
#include "../Rendering/Mesh.h"
#include "ResourceMap.h"
#include <entt/entt.hpp>
#include <memory>
#include <vector>
#include "ChunkCoord.h"

struct Chunk {
    ChunkCoord coord;
    uint32_t terrainSlot = UINT32_MAX;
    bool isGenerated = false;
    bool treesRequested = false;   // NEW — tracks whether we've already queued the CPU tree-generation job for this chunk
    std::vector<entt::entity> treeEntities;
};
