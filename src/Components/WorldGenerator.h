#pragma once
#include <entt/entt.hpp>
#include "GameSettings.h"
#include "ResourceMap.h"

class WorldGenerator {
public:
    // Spawns trees only. Ore is handled entirely by ResourceMap/terrain — no entities.
    static void ScatterTrees(entt::registry &registry, const TerrainSettings &terrainSettings, ResourceMap &resourceMap);

    // Removes all previously-scattered harvestable entities (call before re-scattering on regenerate)
    static void ClearHarvestables(entt::registry& registry);

};
