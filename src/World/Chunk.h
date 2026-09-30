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
    uint32_t treeSlot = UINT32_MAX;   // NEW    std::vector<entt::entity> treeEntities;
    bool isGenerated = false;
    bool treesRequested = false;   // no longer needed for CPU async path, but harmless to leave — see note below
    std::vector<entt::entity> treeEntities;   // no longer needed either, trees aren't entities now — see note below
};
