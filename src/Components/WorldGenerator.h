#pragma once
#include <entt/entt.hpp>
#include "GameSettings.h"
#include "ResourceMap.h"
#include "../Engine/VulkanEngine.h"
#include "../Renderer/MeshRenderer.h"
#include "ChunkCoord.h"
#include <random>
#include "PlacementGrid.h"
class WorldGenerator {
public:
    // Spawns trees only. Ore is handled entirely by ResourceMap/terrain — no entities.
    static void ScatterTrees(entt::registry &registry, const TerrainSettings &terrainSettings, ResourceMap &resourceMap, VulkanEngine *engine, MeshRenderer *meshRenderer);
    static glm::vec3 FindSpawnPoint(const TerrainSettings &terrainSettings, int seed, std::mt19937 &rng);
    // Removes all previously-scattered harvestable entities (call before re-scattering on regenerate)
    static void ClearHarvestables(entt::registry& registry);
    static void ScatterTreesInChunk(entt::registry &registry, ChunkCoord coord, float chunkWorldSize,
                                    const TerrainSettings &terrainSettings, VulkanEngine *engine, MeshRenderer *meshRenderer,
                                    std::vector<entt::entity> &outTreeEntities, PlacementGrid &placementGrid);
};
