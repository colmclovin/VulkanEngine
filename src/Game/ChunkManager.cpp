// ChunkManager.cpp
#include "ChunkManager.h"
#include "../Engine/VulkanEngine.h"
#include "../Components/Components.h"
#include "../World/TerrainGenerator.h"
#include "../World/WorldGenerator.h"
#include "../Components/ChunkBoundsComponent.h"
#include <iostream>
#include <FastNoise/FastNoiseLite.h>
#include "OreDepositMap.h"
#include "../Renderer/TerrainRenderer.h"

ChunkCoord ChunkManager::WorldToChunkCoord(glm::vec3 worldPos) {
    return {
        static_cast<int>(std::floor(worldPos.x / CHUNK_WORLD_SIZE)),
        static_cast<int>(std::floor(worldPos.z / CHUNK_WORLD_SIZE))
    };
}

void ChunkManager::Update(entt::registry &registry, glm::vec3 playerPosition, const TerrainSettings &settings,
                          VulkanEngine *engine, MeshRenderer *meshRenderer, int seed, DepletionMap &depletionMap, PlacementGrid &placementGrid, RemovedTreesMap &removedTreesMap, TerrainRenderer * terrainRenderer) {
    ChunkCoord playerChunk = WorldToChunkCoord(playerPosition);

    std::vector<ChunkCoord> desiredChunks;
    for (int dz = -LOAD_RADIUS_CHUNKS; dz <= LOAD_RADIUS_CHUNKS; dz++) {
        for (int dx = -LOAD_RADIUS_CHUNKS; dx <= LOAD_RADIUS_CHUNKS; dx++) {
            desiredChunks.push_back({ playerChunk.x + dx, playerChunk.z + dz });
        }
    }

    // Request generation for anything not loaded and not already in flight
    for (auto &coord : desiredChunks) {
        if (m_LoadedChunks.find(coord) != m_LoadedChunks.end()) continue;

        std::lock_guard<std::mutex> lock(m_RequestMutex);
        if (m_InFlightRequests.find(coord) != m_InFlightRequests.end()) continue;
        if (m_AwaitingProcessing.find(coord) != m_AwaitingProcessing.end()) continue; // NEW

        m_InFlightRequests.insert(coord);
        m_PendingRequests.push(coord);
        m_RequestCV.notify_one();
    }

    // Turn any finished background work into real entities/GPU resources
    ProcessCompletedChunks(registry, engine, meshRenderer, placementGrid, removedTreesMap, terrainRenderer);

    // Unload chunks no longer in range — unchanged
    std::vector<ChunkCoord> toUnload;
    for (auto &[coord, chunk] : m_LoadedChunks) {
        bool stillDesired = false;
        for (auto &d : desiredChunks) {
            if (d == coord) {
                stillDesired = true;
                break;
            }
        }
        if (!stillDesired) toUnload.push_back(coord);
    }
    for (auto &coord : toUnload) {
        UnloadChunk(coord, registry, engine, terrainRenderer);
    }
}

//void ChunkManager::GenerateChunk(ChunkCoord coord, entt::registry &registry, const TerrainSettings &settings,
//                                 VulkanEngine *engine, MeshRenderer *meshRenderer, int seed, DepletionMap &depletionMap, PlacementGrid &placementGrid) {
//    Chunk chunk;
//    chunk.coord = coord;
//
//    RegenerateChunkMesh(chunk, settings, depletionMap);
//
//    chunk.terrainEntity = registry.create();
//    registry.emplace<TransformComponent>(chunk.terrainEntity);
//    registry.emplace<MeshComponent>(chunk.terrainEntity, chunk.terrainMesh);
//    registry.emplace<NameTag>(chunk.terrainEntity, "Chunk_" + std::to_string(coord.x) + "_" + std::to_string(coord.z));
//
//    WorldGenerator::ScatterTreesInChunk(registry, coord, CHUNK_WORLD_SIZE, settings, engine, meshRenderer, chunk.treeEntities, placementGrid);
//
//    chunk.isGenerated = true;
//    m_LoadedChunks[coord] = chunk;
//}

void ChunkManager::UnloadChunk(ChunkCoord coord, entt::registry& registry, VulkanEngine* engine, TerrainRenderer* terrainRenderer) {
    std::cout << "UnloadChunk called for (" << coord.x << "," << coord.z << ")" << std::endl;
    auto it = m_LoadedChunks.find(coord);
    if (it == m_LoadedChunks.end()) return;

    Chunk& chunk = it->second;

    if (chunk.terrainSlot != UINT32_MAX) {   // CHANGED — was checking/using chunk.terrainEntity
        terrainRenderer->FreeChunkSlot(chunk.terrainSlot);
        std::cout << "  Freed terrain slot " << chunk.terrainSlot << std::endl;
    }

    for (auto& tree : chunk.treeEntities) {
        if (registry.valid(tree)) registry.destroy(tree);
    }

    m_LoadedChunks.erase(it);
}
void ChunkManager::OnResourceDepleted(entt::registry& registry, glm::vec3 worldPos, const TerrainSettings& settings,
    VulkanEngine* engine, MeshRenderer* meshRenderer, DepletionMap& depletionMap,
    TerrainRenderer* terrainRenderer) {   // ADD
    ChunkCoord coord = WorldToChunkCoord(worldPos);
    auto it = m_LoadedChunks.find(coord);
    if (it == m_LoadedChunks.end()) return;

    Chunk& chunk = it->second;
    RegenerateChunkMesh(chunk, settings, depletionMap, terrainRenderer);   // ADD terrainRenderer
    // no more registry.replace<MeshComponent>(...) — terrain has no entity anymore
}

// ChunkManager.cpp
void ChunkManager::RegenerateChunkMesh(Chunk& chunk, const TerrainSettings& settings, const DepletionMap& depletionMap, TerrainRenderer* terrainRenderer) {
    float chunkOriginX = chunk.coord.x * CHUNK_WORLD_SIZE;
    float chunkOriginZ = chunk.coord.z * CHUNK_WORLD_SIZE;
    float cellSize = CHUNK_WORLD_SIZE / (CHUNK_VERTEX_RESOLUTION - 1);

    int paddedRes = CHUNK_VERTEX_RESOLUTION + 2;
    std::vector<glm::vec3> paddedPositions(paddedRes * paddedRes);
    std::vector<glm::vec3> paddedColors(paddedRes * paddedRes);

    for (int z = 0; z < paddedRes; z++) {
        for (int x = 0; x < paddedRes; x++) {
            float worldX = chunkOriginX + (x - 1) * cellSize;
            float worldZ = chunkOriginZ + (z - 1) * cellSize;
            auto sample = TerrainGenerator::SampleTerrain(worldX, worldZ, settings, depletionMap);
            int idx = z * paddedRes + x;
            paddedPositions[idx] = glm::vec3(worldX, sample.height, worldZ);
            paddedColors[idx] = sample.color;
        }
    }

    std::vector<glm::vec3> paddedNormals(paddedRes * paddedRes, glm::vec3(0.0f));
    for (int z = 0; z < paddedRes - 1; z++) {
        for (int x = 0; x < paddedRes - 1; x++) {
            int tl = z * paddedRes + x;
            int tr = tl + 1;
            int bl = (z + 1) * paddedRes + x;
            int br = bl + 1;

            glm::vec3 faceNormal = glm::normalize(glm::cross(
                paddedPositions[bl] - paddedPositions[tl],
                paddedPositions[tr] - paddedPositions[tl]));

            paddedNormals[tl] += faceNormal;
            paddedNormals[tr] += faceNormal;
            paddedNormals[bl] += faceNormal;
            paddedNormals[br] += faceNormal;
        }
    }

    std::vector<Vertex> vertices(CHUNK_VERTEX_RESOLUTION * CHUNK_VERTEX_RESOLUTION);
    for (int z = 0; z < CHUNK_VERTEX_RESOLUTION; z++) {
        for (int x = 0; x < CHUNK_VERTEX_RESOLUTION; x++) {
            int paddedIdx = (z + 1) * paddedRes + (x + 1);
            int realIdx = z * CHUNK_VERTEX_RESOLUTION + x;

            Vertex& v = vertices[realIdx];
            v.position = paddedPositions[paddedIdx];
            v.normal = glm::normalize(paddedNormals[paddedIdx]);
            v.texCoord = glm::vec2((float)x / (CHUNK_VERTEX_RESOLUTION - 1), (float)z / (CHUNK_VERTEX_RESOLUTION - 1));
            v.color = paddedColors[paddedIdx];
        }
    }

    std::vector<uint32_t> indices;
    for (int z = 0; z < CHUNK_VERTEX_RESOLUTION - 1; z++) {
        for (int x = 0; x < CHUNK_VERTEX_RESOLUTION - 1; x++) {
            int topLeft = z * CHUNK_VERTEX_RESOLUTION + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * CHUNK_VERTEX_RESOLUTION + x;
            int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    // Re-upload into the SAME slot this chunk already owns — no allocation, just overwrite
    glm::vec3 boundsCenter(
        chunk.coord.x * CHUNK_WORLD_SIZE + CHUNK_WORLD_SIZE * 0.5f,
        0.0f,
        chunk.coord.z * CHUNK_WORLD_SIZE + CHUNK_WORLD_SIZE * 0.5f);
    float boundsRadius = CHUNK_WORLD_SIZE * 0.75f;

    terrainRenderer->UploadChunk(chunk.terrainSlot, vertices, indices, boundsCenter, boundsRadius);
}

void ChunkManager::StartWorkerThread(const TerrainSettings& settings, int seed) {
    m_ShouldStop = false;
    for (int i = 0; i < WORKER_THREAD_COUNT; i++) {
        m_WorkerThreads.emplace_back(&ChunkManager::WorkerLoop, this, settings, seed);
    }
}
void ChunkManager::StopWorkerThread() {
    m_ShouldStop = true;
    m_RequestCV.notify_all();   // wake ALL waiting threads, not just one
    for (auto& t : m_WorkerThreads) {
        if (t.joinable()) t.join();
    }
    m_WorkerThreads.clear();
}

void ChunkManager::WorkerLoop(TerrainSettings settings, int seed) {
    while (!m_ShouldStop) {
        ChunkCoord coord;
        {
            std::unique_lock<std::mutex> lock(m_RequestMutex);
            m_RequestCV.wait(lock, [this] { return !m_PendingRequests.empty() || m_ShouldStop; });
            if (m_ShouldStop) return;

            coord = m_PendingRequests.front();
            m_PendingRequests.pop();
        }

        ChunkGenerationResult result;
        result.coord = coord;
        result.terrainData = GenerateTerrainDataOnly(coord, settings);
        result.treeCandidates = GenerateTreeCandidatesOnly(coord, settings);

        {
            std::lock_guard<std::mutex> lock(m_ResultMutex);
            m_CompletedResults.push(std::move(result));
        }
        {
            std::lock_guard<std::mutex> lock(m_RequestMutex);
            m_InFlightRequests.erase(coord);
            m_AwaitingProcessing.insert(coord); // NEW — closes the gap between "done generating" and "actually in m_LoadedChunks"
        }
    }
}


void ChunkManager::ProcessCompletedChunks(entt::registry& registry, VulkanEngine* engine, MeshRenderer* meshRenderer,
    PlacementGrid& placementGrid, RemovedTreesMap& removedTreesMap,
    TerrainRenderer* terrainRenderer) {   // NEW parameter
    const int MAX_PER_FRAME = 2;

    for (int i = 0; i < MAX_PER_FRAME; i++) {
        ChunkGenerationResult result;
        {
            std::lock_guard<std::mutex> lock(m_ResultMutex);
            if (m_CompletedResults.empty()) return;
            result = std::move(m_CompletedResults.front());
            m_CompletedResults.pop();
        }

        Chunk chunk;
        chunk.coord = result.coord;

        // NEW — allocate a slot and upload via TerrainRenderer, instead of creating a Mesh/MeshComponent
        auto slot = terrainRenderer->AllocateChunkSlot();
        if (!slot) {
            std::cerr << "TerrainRenderer: out of chunk slots! Chunk (" << result.coord.x << "," << result.coord.z << ") not rendered." << std::endl;
            continue;
        }
        chunk.terrainSlot = *slot;   // NEW field on Chunk, see below

        glm::vec3 boundsCenter(
            chunk.coord.x * CHUNK_WORLD_SIZE + CHUNK_WORLD_SIZE * 0.5f,
            0.0f,
            chunk.coord.z * CHUNK_WORLD_SIZE + CHUNK_WORLD_SIZE * 0.5f);
        float boundsRadius = CHUNK_WORLD_SIZE * 0.75f;

        terrainRenderer->UploadChunk(*slot, result.terrainData.vertices, result.terrainData.indices, boundsCenter, boundsRadius);

        // NOTE: no more terrainEntity, no ChunkBoundsComponent, no MeshComponent for terrain at all —
        // the chunk's visual representation is now entirely GPU-side, tracked only via chunk.terrainSlot.

        auto treeMesh = WorldGenerator::GetTreeMeshCache(engine, meshRenderer);
        int index = 0;
        for (auto& candidate : result.treeCandidates) {
            if (removedTreesMap.IsRemoved(result.coord, index)) {
                index++;
                continue;
            }

            auto entity = registry.create();
            auto& transform = registry.emplace<TransformComponent>(entity);
            transform.Position = candidate.position;
            registry.emplace<MeshComponent>(entity, treeMesh);
            registry.emplace<BoundsComponent>(entity, BoundsComponent{ glm::vec3(0.5f, 2.0f, 0.5f) });
            registry.emplace<TreeOriginComponent>(entity, TreeOriginComponent{ result.coord, index });

            HarvestableComponent harvest;
            harvest.yieldItem = ItemId::Wood;
            harvest.health = 100.0f;
            harvest.maxHealth = 100.0f;
            harvest.yieldPerHit = 1;
            harvest.yieldOnDestroy = 5;
            registry.emplace<HarvestableComponent>(entity, harvest);
            registry.emplace<NameTag>(entity, "Tree");

            GridCoord gridCoord = PlacementGrid::WorldToGrid(candidate.position, 1.0f);
            placementGrid.Register(gridCoord, entity);

            chunk.treeEntities.push_back(entity);
            index++;
        }

        chunk.isGenerated = true;
        m_LoadedChunks[result.coord] = chunk;
    }
}

// ChunkManager.cpp
ChunkManager::GeneratedVertexData ChunkManager::GenerateTerrainDataOnly(ChunkCoord coord, const TerrainSettings& settings) {
    float chunkOriginX = coord.x * CHUNK_WORLD_SIZE;
    float chunkOriginZ = coord.z * CHUNK_WORLD_SIZE;
    float cellSize = CHUNK_WORLD_SIZE / (CHUNK_VERTEX_RESOLUTION - 1);

    GeneratedVertexData data;

    int paddedRes = CHUNK_VERTEX_RESOLUTION + 2;
    std::vector<glm::vec3> paddedPositions(paddedRes * paddedRes);
    std::vector<glm::vec3> paddedColors(paddedRes * paddedRes);   // NEW — store color alongside position

    DepletionMap emptyDepletionMap;

    for (int z = 0; z < paddedRes; z++) {
        for (int x = 0; x < paddedRes; x++) {
            float worldX = chunkOriginX + (x - 1) * cellSize;
            float worldZ = chunkOriginZ + (z - 1) * cellSize;
            auto sample = TerrainGenerator::SampleTerrain(worldX, worldZ, settings, emptyDepletionMap);   // ONLY call, per position
            int idx = z * paddedRes + x;
            paddedPositions[idx] = glm::vec3(worldX, sample.height, worldZ);
            paddedColors[idx] = sample.color;   // NEW — stored for reuse below
        }
    }

    std::vector<glm::vec3> paddedNormals(paddedRes * paddedRes, glm::vec3(0.0f));
    for (int z = 0; z < paddedRes - 1; z++) {
        for (int x = 0; x < paddedRes - 1; x++) {
            int tl = z * paddedRes + x;
            int tr = tl + 1;
            int bl = (z + 1) * paddedRes + x;
            int br = bl + 1;

            glm::vec3 faceNormal = glm::normalize(glm::cross(
                paddedPositions[bl] - paddedPositions[tl],
                paddedPositions[tr] - paddedPositions[tl]));

            paddedNormals[tl] += faceNormal;
            paddedNormals[tr] += faceNormal;
            paddedNormals[bl] += faceNormal;
            paddedNormals[br] += faceNormal;
        }
    }

    data.vertices.resize(CHUNK_VERTEX_RESOLUTION * CHUNK_VERTEX_RESOLUTION);
    for (int z = 0; z < CHUNK_VERTEX_RESOLUTION; z++) {
        for (int x = 0; x < CHUNK_VERTEX_RESOLUTION; x++) {
            int paddedIdx = (z + 1) * paddedRes + (x + 1);
            int realIdx = z * CHUNK_VERTEX_RESOLUTION + x;

            Vertex& v = data.vertices[realIdx];
            v.position = paddedPositions[paddedIdx];
            v.normal = glm::normalize(paddedNormals[paddedIdx]);
            v.texCoord = glm::vec2((float)x / (CHUNK_VERTEX_RESOLUTION - 1), (float)z / (CHUNK_VERTEX_RESOLUTION - 1));
            v.color = paddedColors[paddedIdx];   // CHANGED — reused from padded grid, no second SampleTerrain call
        }
    }

    for (int z = 0; z < CHUNK_VERTEX_RESOLUTION - 1; z++) {
        for (int x = 0; x < CHUNK_VERTEX_RESOLUTION - 1; x++) {
            int topLeft = z * CHUNK_VERTEX_RESOLUTION + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * CHUNK_VERTEX_RESOLUTION + x;
            int bottomRight = bottomLeft + 1;

            data.indices.push_back(topLeft);
            data.indices.push_back(bottomLeft);
            data.indices.push_back(topRight);
            data.indices.push_back(topRight);
            data.indices.push_back(bottomLeft);
            data.indices.push_back(bottomRight);
        }
    }

    return data;
}

std::vector<ChunkManager::TreeCandidate> ChunkManager::GenerateTreeCandidatesOnly(ChunkCoord coord, const TerrainSettings& settings) {
    std::vector<TreeCandidate> candidates;

    float chunkOriginX = coord.x * CHUNK_WORLD_SIZE;
    float chunkOriginZ = coord.z * CHUNK_WORLD_SIZE;
    float worldExtentZ = settings.gridDepth * settings.cellSize;

    // Cached per-thread — configured once, reused across every chunk this thread ever processes
    static thread_local FastNoiseLite treeJitterNoise;
    static thread_local int lastSeed = -1;
    if (lastSeed != settings.seed) {
        treeJitterNoise.SetSeed(settings.seed + 4000);
        treeJitterNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        treeJitterNoise.SetFrequency(0.4f);
        lastSeed = settings.seed;
    }

    // Thread-safe RNG for jitter, replacing rand()/RAND_MAX — each thread gets its own independent generator
    static thread_local std::mt19937 treeRng(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    std::uniform_real_distribution<float> jitterRange(-0.5f, 0.5f);

    float sampleSpacing = 8.0f;

    for (float x = 0.0f; x < CHUNK_WORLD_SIZE; x += sampleSpacing) {
        for (float z = 0.0f; z < CHUNK_WORLD_SIZE; z += sampleSpacing) {
            float worldX = chunkOriginX + x;
            float worldZ = chunkOriginZ + z;

            BiomeId biome = BiomeMap::GetBiomeAt(worldX, worldZ, worldExtentZ, settings.seed);
            if (biome == BiomeId::Lake) continue;

            const BiomeDef& def = BiomeDatabase::Get(biome);
            if (def.treeDensity <= 0.0f) continue;

            auto deposit = OreDepositMap::GetDepositAt(worldX, worldZ, settings.seed);
            if (deposit) continue;

            float aridity = BiomeMap::GetAridity(worldX, worldZ, settings.seed);
            float wetness = 1.0f - aridity;
            float effectiveDensity = def.treeDensity * wetness;

            float roll = treeJitterNoise.GetNoise(worldX, worldZ);
            float threshold = 1.0f - (effectiveDensity * 2.0f);
            if (roll < threshold) continue;

            float jitterX = jitterRange(treeRng) * sampleSpacing;   // CHANGED — thread-safe, no more rand()
            float jitterZ = jitterRange(treeRng) * sampleSpacing;   // CHANGED
            float finalX = worldX + jitterX;
            float finalZ = worldZ + jitterZ;

            auto sample = TerrainGenerator::SampleHeight(finalX, finalZ, settings);

            candidates.push_back({ glm::vec3(finalX, sample, finalZ) });
        }
    }

    return candidates;
}

void ChunkManager::RequestInitialChunksBlocking(glm::vec3 playerPos, const TerrainSettings &settings,
                                                VulkanEngine *engine, MeshRenderer *meshRenderer, int seed,
                                                DepletionMap &depletionMap, RemovedTreesMap &removedTreesMap,
                                                entt::registry &registry, PlacementGrid &placementGrid, TerrainRenderer *terrainRenderer) {
    ChunkCoord playerChunk = WorldToChunkCoord(playerPos);
    std::vector<ChunkCoord> desired;
    for (int dz = -LOAD_RADIUS_CHUNKS; dz <= LOAD_RADIUS_CHUNKS; dz++)
        for (int dx = -LOAD_RADIUS_CHUNKS; dx <= LOAD_RADIUS_CHUNKS; dx++)
            desired.push_back({ playerChunk.x + dx, playerChunk.z + dz });

    for (auto &coord : desired) {
        std::lock_guard<std::mutex> lock(m_RequestMutex);
        m_InFlightRequests.insert(coord);
        m_PendingRequests.push(coord);
        m_RequestCV.notify_one();
    }

    while (m_LoadedChunks.size() < desired.size()) {
        ProcessCompletedChunks(registry, engine, meshRenderer, placementGrid, removedTreesMap, terrainRenderer);
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // avoid busy-spinning
    }
}
void ChunkManager::BeginInitialLoad(glm::vec3 playerPos, const TerrainSettings &settings, int seed) {
    ChunkCoord playerChunk = WorldToChunkCoord(playerPos);
    std::vector<ChunkCoord> desired;
    for (int dz = -LOAD_RADIUS_CHUNKS; dz <= LOAD_RADIUS_CHUNKS; dz++)
        for (int dx = -LOAD_RADIUS_CHUNKS; dx <= LOAD_RADIUS_CHUNKS; dx++)
            desired.push_back({ playerChunk.x + dx, playerChunk.z + dz });

    m_InitialLoadTarget = desired.size();

    for (auto &coord : desired) {
        std::lock_guard<std::mutex> lock(m_RequestMutex);
        m_InFlightRequests.insert(coord);
        m_PendingRequests.push(coord);
        m_RequestCV.notify_one();
    }
}

bool ChunkManager::IsInitialLoadComplete() const {
    return m_LoadedChunks.size() >= m_InitialLoadTarget;
}

float ChunkManager::GetInitialLoadProgress() const {
    if (m_InitialLoadTarget == 0) return 1.0f;
    return glm::clamp((float)m_LoadedChunks.size() / (float)m_InitialLoadTarget, 0.0f, 1.0f);
}