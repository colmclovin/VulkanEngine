// ChunkManager.h
#pragma once
#include <glm/glm.hpp>
#include <optional>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>
#include "../World/Chunk.h"
#include "../World/ChunkCoord.h"
#include "../World/DepletionMap.h"
#include "../World/RemovedTreesMap.h"
#include "../Utils/GameSettings.h"

class TerrainRenderer;
class TreeRenderer;
class ChunkGenerator;

// Decides which terrain chunks should be loaded around the camera/player and owns their GPU slots.
// Chunk terrain and trees are generated entirely on the GPU (see ChunkGenerator / chunk_generate_*.comp):
// Update() allocates slots and queues chunks, RecordPendingGeneration() records the compute work.
class ChunkManager {
public:
    static constexpr float CHUNK_WORLD_SIZE = 32.0f;   // world units per chunk edge
    static constexpr int CHUNK_VERTEX_RESOLUTION = 16; // vertices per chunk edge
    static constexpr float MESH_CELL_SIZE = CHUNK_WORLD_SIZE / (CHUNK_VERTEX_RESOLUTION - 1); // spacing between terrain vertices
    static constexpr int LOAD_RADIUS_CHUNKS = 16;      // how many chunks around the player stay loaded
    static constexpr float MAX_ZOOM_FACTOR = 1.8f;     // load radius multiplier at max zoom-out

    static ChunkCoord WorldToChunkCoord(glm::vec3 worldPos);

    // Loads chunks in a forward-biased radius around center (wider when zoomed out) and unloads the rest.
    void Update(glm::vec3 center, glm::vec3 cameraForward, float isoDistance, TerrainRenderer *terrainRenderer,
                TreeRenderer *treeRenderer);

    // Records GPU generation for every chunk queued since the last call. Call inside a frame's command buffer.
    void RecordPendingGeneration(VkCommandBuffer commandBuffer, ChunkGenerator *generator, TerrainRenderer *terrainRenderer,
                                 TreeRenderer *treeRenderer, const TerrainSettings &settings,
                                 const DepletionMap &depletionMap, const RemovedTreesMap &removedTreesMap);

    // Re-runs GPU generation for a loaded chunk (after mining ore or chopping a tree), reusing its slots.
    void RequestChunkRegeneration(ChunkCoord coord);

    // Loading screen: how many chunks must exist before play starts.
    void BeginInitialLoad(glm::vec3 playerPos, glm::vec3 cameraForward);
    bool IsInitialLoadComplete() const;
    float GetInitialLoadProgress() const;

    std::optional<uint32_t> GetTreeSlotForChunk(ChunkCoord coord) const;

    void UnloadAll(TerrainRenderer *terrainRenderer, TreeRenderer *treeRenderer);
    void Reset() {
        m_LoadedChunks.clear();
        m_PendingGpuGeneration.clear();
        m_InitialLoadTarget = 0;
    }

private:
    void UnloadChunk(ChunkCoord coord, TerrainRenderer *terrainRenderer, TreeRenderer *treeRenderer);

    std::unordered_map<ChunkCoord, Chunk, ChunkCoordHash> m_LoadedChunks;
    std::vector<ChunkCoord> m_PendingGpuGeneration; // filled by Update/RequestChunkRegeneration, drained by RecordPendingGeneration
    size_t m_InitialLoadTarget = 0;
};
