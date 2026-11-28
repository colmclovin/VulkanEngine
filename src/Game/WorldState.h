#pragma once
#include "../World/PlacementGrid.h"
#include "../World/DepletionMap.h"
#include "../World/RemovedTreesMap.h"
#include "../World/TreeHealthMap.h"
#include "TechState.h"
#include "FogOfWar.h"

// Per-save world data that lives outside the ECS registry.
// Everything here is wiped by `world = WorldState{}` when a game is reset or loaded.
struct WorldState {
    PlacementGrid placementGrid;     // which grid cells are occupied by buildings
    TechState techState;
    DepletionMap depletionMap;       // how much ore has been mined from each cell (saved)
    RemovedTreesMap removedTrees;    // trees chopped down, per chunk (saved)
    TreeHealthMap treeHealth;        // damage on trees that are still standing (not saved)
    FogOfWar fog;                    // explored/visible state (not saved yet)
};
