// ChunkManager.h
#pragma once
#include "../World/Chunk.h"
#include "../World/ChunkCoord.h"
#include "../Utils/GameSettings.h"
#include <entt/entt.hpp>
#include <unordered_map>
#include "../World/DepletionMap.h"
#include "../World/PlacementGrid.h"
#include "../World/RemovedTreesMap.h"
#include "../Rendering/Vertex.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <unordered_set>   // add if not already present
#include "../Renderer/TerrainRenderer.h"
#include "ChunkGenerator.h"

class VulkanEngine;
class MeshRenderer;
class TerrainRenderer;
class TreeRenderer;

struct GpuGenerationRequest {
    ChunkCoord coord;
};

class ChunkManager {
public:
    static constexpr int WORKER_THREAD_COUNT = 8;   // tune based on your CPU's core count
    static constexpr float CHUNK_WORLD_SIZE = 32.0f; // world units per chunk edge
    static constexpr int CHUNK_VERTEX_RESOLUTION = 16; // vertices per chunk edge (independent of world cellSize)
    static constexpr int LOAD_RADIUS_CHUNKS = 16; // how many chunks around the player stay loaded
    static constexpr float MAX_ZOOM_FACTOR = 1.8f;

    struct ChunkGenerationJob {
        ChunkCoord coord;
    };

    struct GeneratedVertexData {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };

    struct TreeCandidate {
        glm::vec3 position;
    };

    struct ChunkGenerationResult {
        ChunkCoord coord;
        GeneratedVertexData terrainData;
        std::vector<TreeCandidate> treeCandidates;
    };
    void StartWorkerThread(const TerrainSettings &settings, int seed);
    void StopWorkerThread();
    void ProcessCompletedChunks(entt::registry& registry, VulkanEngine* engine, MeshRenderer* meshRenderer,
        PlacementGrid& placementGrid, RemovedTreesMap& removedTreesMap);

    void OnResourceDepleted(entt::registry& registry, glm::vec3 worldPos, const TerrainSettings& settings,
        VulkanEngine* engine, MeshRenderer* meshRenderer, DepletionMap& depletionMap,
        TerrainRenderer* terrainRenderer);
    static ChunkCoord WorldToChunkCoord(glm::vec3 worldPos);
    void RegenerateChunkMesh(Chunk& chunk, const TerrainSettings& settings, const DepletionMap& depletionMap, TerrainRenderer* terrainRenderer);

    void RequestInitialChunksBlocking(glm::vec3 playerPos, const TerrainSettings& settings,
        VulkanEngine* engine, MeshRenderer* meshRenderer, int seed,
        DepletionMap& depletionMap, RemovedTreesMap& removedTreesMap,
        entt::registry& registry, PlacementGrid& placementGrid, TerrainRenderer* terrainRenderer);
    void BeginInitialLoad(glm::vec3 playerPos, glm::vec3 cameraForward, const TerrainSettings &settings, int seed);
    bool IsInitialLoadComplete() const;
    float GetInitialLoadProgress() const;
    void Update(entt::registry& registry, glm::vec3 playerPosition, const TerrainSettings& settings,
        VulkanEngine* engine, MeshRenderer* meshRenderer, int seed, DepletionMap& depletionMap,
                PlacementGrid &placementGrid, TerrainRenderer *terrainRenderer, RemovedTreesMap &removedTreesMap, glm::vec3 cameraForward, float isoDistance, TreeRenderer* treeRenderer);

    void RecordPendingGeneration(VkCommandBuffer commandBuffer, ChunkGenerator* generator,
        TerrainRenderer* terrainRenderer, TreeRenderer* treeRenderer, const TerrainSettings& settings);
   
    void Reset() {
        m_LoadedChunks.clear();
        {
            std::lock_guard<std::mutex> lock(m_RequestMutex);
            while (!m_PendingRequests.empty())
                m_PendingRequests.pop();
            m_InFlightRequests.clear();
            m_AwaitingProcessing.clear();
        }
        {
            std::lock_guard<std::mutex> lock(m_ResultMutex);
            while (!m_CompletedResults.empty())
                m_CompletedResults.pop();
        }
    }
    ChunkManager::GeneratedVertexData GenerateTerrainDataOnly(ChunkCoord coord, const TerrainSettings& settings); // pure CPU, no GPU/registry

private:
    size_t m_InitialLoadTarget = 0;
    void GenerateChunk(ChunkCoord coord, entt::registry &registry, const TerrainSettings &settings,
                       VulkanEngine *engine, MeshRenderer *meshRenderer, int seed, DepletionMap &depletionMap, PlacementGrid &placementGrid);
    void UnloadChunk(ChunkCoord coord, entt::registry& registry, VulkanEngine* engine, TerrainRenderer* terrainRenderer, TreeRenderer* treeRenderer);

    std::unordered_map<ChunkCoord, Chunk, ChunkCoordHash> m_LoadedChunks;
    std::unordered_set<ChunkCoord, ChunkCoordHash> m_AwaitingProcessing;
    void WorkerLoop(TerrainSettings settings, int seed); // takes settings BY VALUE — a safe snapshot, no shared-state races
    std::vector<ChunkManager::TreeCandidate> GenerateTreeCandidatesOnly(ChunkCoord coord, const TerrainSettings &settings);

    std::vector<std::thread> m_WorkerThreads;   // CHANGED — was a single std::thread
    std::atomic<bool> m_ShouldStop{ false };

    std::mutex m_RequestMutex;
    std::condition_variable m_RequestCV;
    std::queue<ChunkCoord> m_PendingRequests;
    std::unordered_set<ChunkCoord, ChunkCoordHash> m_InFlightRequests; // prevents double-queuing the same chunk
    std::vector<GpuGenerationRequest> m_PendingGpuGeneration;   // filled by Update(), drained/recorded by RenderFrame
    std::mutex m_ResultMutex;
    std::queue<ChunkGenerationResult> m_CompletedResults;


};