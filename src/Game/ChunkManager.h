// ChunkManager.h
#pragma once
#include "../Components/Chunk.h"
#include "../Components/ChunkCoord.h"
#include "../Components/GameSettings.h"
#include <entt/entt.hpp>
#include <unordered_map>
#include "../Components/DepletionMap.h"
#include "../Components/PlacementGrid.h"
#include "../Components/Vertex.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <unordered_set>   // add if not already present



class VulkanEngine;
class MeshRenderer;

class ChunkManager {
public:
    static constexpr float CHUNK_WORLD_SIZE = 32.0f; // world units per chunk edge
    static constexpr int CHUNK_VERTEX_RESOLUTION = 32; // vertices per chunk edge (independent of world cellSize)
    static constexpr int LOAD_RADIUS_CHUNKS = 6; // how many chunks around the player stay loaded

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
    void ProcessCompletedChunks(entt::registry &registry, VulkanEngine *engine, MeshRenderer *meshRenderer, PlacementGrid &placementGrid);

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
    std::unordered_set<ChunkCoord, ChunkCoordHash> m_AwaitingProcessing;
    void WorkerLoop(TerrainSettings settings, int seed); // takes settings BY VALUE — a safe snapshot, no shared-state races
    ChunkManager::GeneratedVertexData GenerateTerrainDataOnly(ChunkCoord coord, const TerrainSettings &settings); // pure CPU, no GPU/registry
    std::vector<ChunkManager::TreeCandidate> GenerateTreeCandidatesOnly(ChunkCoord coord, const TerrainSettings &settings);

    std::thread m_WorkerThread;
    std::atomic<bool> m_ShouldStop{ false };

    std::mutex m_RequestMutex;
    std::condition_variable m_RequestCV;
    std::queue<ChunkCoord> m_PendingRequests;
    std::unordered_set<ChunkCoord, ChunkCoordHash> m_InFlightRequests; // prevents double-queuing the same chunk

    std::mutex m_ResultMutex;
    std::queue<ChunkGenerationResult> m_CompletedResults;


};