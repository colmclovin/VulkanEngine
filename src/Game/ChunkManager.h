// ChunkManager.h
#pragma once
#include "../Components/Chunk.h"
#include "../Components/ChunkCoord.h"
#include "../Components/GameSettings.h"
#include <entt/entt.hpp>
#include <unordered_map>
#include "../Components/DepletionMap.h"
#include "../Components/PlacementGrid.h"
class VulkanEngine;
class MeshRenderer;

class ChunkManager {
public:
    static constexpr float CHUNK_WORLD_SIZE = 32.0f; // world units per chunk edge
    static constexpr int CHUNK_VERTEX_RESOLUTION = 32; // vertices per chunk edge (independent of world cellSize)
    static constexpr int LOAD_RADIUS_CHUNKS = 6; // how many chunks around the player stay loaded

    void Update(entt::registry &registry, glm::vec3 playerPosition, const TerrainSettings &settings,
                VulkanEngine *engine, MeshRenderer *meshRenderer, int seed, DepletionMap &depletionMap, PlacementGrid &placementGrid);
    void OnResourceDepleted(entt::registry &registry, glm::vec3 worldPos, const TerrainSettings &settings,
                            VulkanEngine *engine, MeshRenderer *meshRenderer, DepletionMap &depletionMap);
    static ChunkCoord WorldToChunkCoord(glm::vec3 worldPos);
    void RegenerateChunkMesh(Chunk &chunk, const TerrainSettings &settings, const DepletionMap &depletionMap);

private:
    void GenerateChunk(ChunkCoord coord, entt::registry &registry, const TerrainSettings &settings,
                       VulkanEngine *engine, MeshRenderer *meshRenderer, int seed, DepletionMap &depletionMap, PlacementGrid &placementGrid);
    void UnloadChunk(ChunkCoord coord, entt::registry &registry, VulkanEngine *engine);

    std::unordered_map<ChunkCoord, Chunk, ChunkCoordHash> m_LoadedChunks;
};