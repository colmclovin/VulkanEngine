// ChunkManager.cpp
#include "ChunkManager.h"
#include "../Engine/VulkanEngine.h"
#include "../Components/Components.h"
#include "../Components/TerrainGenerator.h"
#include "../Components/WorldGenerator.h"
#include <iostream>
#include <FastNoise/FastNoiseLite.h>
#include "OreDepositMap.h"

ChunkCoord ChunkManager::WorldToChunkCoord(glm::vec3 worldPos) {
    return {
        static_cast<int>(std::floor(worldPos.x / CHUNK_WORLD_SIZE)),
        static_cast<int>(std::floor(worldPos.z / CHUNK_WORLD_SIZE))
    };
}

void ChunkManager::Update(entt::registry &registry, glm::vec3 playerPosition, const TerrainSettings &settings,
                          VulkanEngine *engine, MeshRenderer *meshRenderer, int seed, DepletionMap &depletionMap, PlacementGrid &placementGrid) {
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
    ProcessCompletedChunks(registry, engine, meshRenderer, placementGrid);

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
        UnloadChunk(coord, registry, engine);
    }
}

void ChunkManager::GenerateChunk(ChunkCoord coord, entt::registry &registry, const TerrainSettings &settings,
                                 VulkanEngine *engine, MeshRenderer *meshRenderer, int seed, DepletionMap &depletionMap, PlacementGrid &placementGrid) {
    Chunk chunk;
    chunk.coord = coord;

    RegenerateChunkMesh(chunk, settings, depletionMap);

    chunk.terrainEntity = registry.create();
    registry.emplace<TransformComponent>(chunk.terrainEntity);
    registry.emplace<MeshComponent>(chunk.terrainEntity, chunk.terrainMesh);
    registry.emplace<NameTag>(chunk.terrainEntity, "Chunk_" + std::to_string(coord.x) + "_" + std::to_string(coord.z));

    WorldGenerator::ScatterTreesInChunk(registry, coord, CHUNK_WORLD_SIZE, settings, engine, meshRenderer, chunk.treeEntities, placementGrid);

    chunk.isGenerated = true;
    m_LoadedChunks[coord] = chunk;
}

void ChunkManager::UnloadChunk(ChunkCoord coord, entt::registry &registry, VulkanEngine *engine) {
    std::cout << "Unloading chunk (" << coord.x << "," << coord.z << ")" << std::endl;
    auto it = m_LoadedChunks.find(coord);
    if (it == m_LoadedChunks.end()) return;

    Chunk &chunk = it->second;

   engine->WaitIdle(); // ADD — ensure no in-flight command buffer still references this chunk's GPU resources


    if (registry.valid(chunk.terrainEntity)) {
        chunk.terrainMesh->DestroyGPUResources(engine->GetDevice());
        registry.destroy(chunk.terrainEntity);
    }
    for (auto &tree : chunk.treeEntities) {
        if (registry.valid(tree)) registry.destroy(tree);
    }

    m_LoadedChunks.erase(it);
}
void ChunkManager::OnResourceDepleted(entt::registry &registry, glm::vec3 worldPos, const TerrainSettings &settings,
                                      VulkanEngine *engine, MeshRenderer *meshRenderer, DepletionMap &depletionMap) {
    ChunkCoord coord = WorldToChunkCoord(worldPos);
    auto it = m_LoadedChunks.find(coord);
    if (it == m_LoadedChunks.end()) return;

    Chunk &chunk = it->second;
    engine->WaitIdle();
    chunk.terrainMesh->DestroyGPUResources(engine->GetDevice());

    RegenerateChunkMesh(chunk, settings, depletionMap);

    registry.replace<MeshComponent>(chunk.terrainEntity, chunk.terrainMesh);
}

// ChunkManager.cpp
void ChunkManager::RegenerateChunkMesh(Chunk &chunk, const TerrainSettings &settings, const DepletionMap &depletionMap) {
    float chunkOriginX = chunk.coord.x * CHUNK_WORLD_SIZE;
    float chunkOriginZ = chunk.coord.z * CHUNK_WORLD_SIZE;
    float cellSize = CHUNK_WORLD_SIZE / (CHUNK_VERTEX_RESOLUTION - 1);

    auto mesh = std::make_shared<Mesh>();

    int paddedRes = CHUNK_VERTEX_RESOLUTION + 2;
    std::vector<glm::vec3> paddedPositions(paddedRes * paddedRes);

    for (int z = 0; z < paddedRes; z++) {
        for (int x = 0; x < paddedRes; x++) {
            float worldX = chunkOriginX + (x - 1) * cellSize;
            float worldZ = chunkOriginZ + (z - 1) * cellSize;
            auto sample = TerrainGenerator::SampleTerrain(worldX, worldZ, settings, depletionMap);
            paddedPositions[z * paddedRes + x] = glm::vec3(worldX, sample.height, worldZ);
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

    mesh->Vertices.resize(CHUNK_VERTEX_RESOLUTION * CHUNK_VERTEX_RESOLUTION);
    for (int z = 0; z < CHUNK_VERTEX_RESOLUTION; z++) {
        for (int x = 0; x < CHUNK_VERTEX_RESOLUTION; x++) {
            int paddedIdx = (z + 1) * paddedRes + (x + 1);
            int realIdx = z * CHUNK_VERTEX_RESOLUTION + x;

            float worldX = chunkOriginX + x * cellSize;
            float worldZ = chunkOriginZ + z * cellSize;
            auto sample = TerrainGenerator::SampleTerrain(worldX, worldZ, settings, depletionMap);

            Vertex &v = mesh->Vertices[realIdx];
            v.position = paddedPositions[paddedIdx];
            v.normal = glm::normalize(paddedNormals[paddedIdx]);
            v.texCoord = glm::vec2((float)x / (CHUNK_VERTEX_RESOLUTION - 1), (float)z / (CHUNK_VERTEX_RESOLUTION - 1));
            v.color = sample.color;
        }
    }

    for (int z = 0; z < CHUNK_VERTEX_RESOLUTION - 1; z++) {
        for (int x = 0; x < CHUNK_VERTEX_RESOLUTION - 1; x++) {
            int topLeft = z * CHUNK_VERTEX_RESOLUTION + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * CHUNK_VERTEX_RESOLUTION + x;
            int bottomRight = bottomLeft + 1;

            mesh->Indices.push_back(topLeft);
            mesh->Indices.push_back(bottomLeft);
            mesh->Indices.push_back(topRight);
            mesh->Indices.push_back(topRight);
            mesh->Indices.push_back(bottomLeft);
            mesh->Indices.push_back(bottomRight);
        }
    }

    chunk.terrainMesh = mesh;
}

void ChunkManager::StartWorkerThread(const TerrainSettings &settings, int seed) {
    m_ShouldStop = false;
    m_WorkerThread = std::thread(&ChunkManager::WorkerLoop, this, settings, seed);
}

void ChunkManager::StopWorkerThread() {
    m_ShouldStop = true;
    m_RequestCV.notify_all();
    if (m_WorkerThread.joinable()) {
        m_WorkerThread.join();
    }
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


void ChunkManager::ProcessCompletedChunks(entt::registry &registry, VulkanEngine *engine, MeshRenderer *meshRenderer, PlacementGrid &placementGrid) {
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

        auto mesh = std::make_shared<Mesh>();
        mesh->Vertices = std::move(result.terrainData.vertices);
        mesh->Indices = std::move(result.terrainData.indices);
        mesh->UploadToGPU(engine);
        chunk.terrainMesh = mesh;

        chunk.terrainEntity = registry.create();
        registry.emplace<TransformComponent>(chunk.terrainEntity);
        registry.emplace<MeshComponent>(chunk.terrainEntity, mesh);
        registry.emplace<NameTag>(chunk.terrainEntity, "Chunk_" + std::to_string(result.coord.x) + "_" + std::to_string(result.coord.z));

        auto treeMesh = WorldGenerator::GetTreeMeshCache(engine, meshRenderer); // needs a small accessor, see below
        for (auto &candidate : result.treeCandidates) {
            auto entity = registry.create();
            auto &transform = registry.emplace<TransformComponent>(entity);
            transform.Position = candidate.position;
            registry.emplace<MeshComponent>(entity, treeMesh);
            registry.emplace<BoundsComponent>(entity, BoundsComponent{ glm::vec3(0.5f, 2.0f, 0.5f) });

            HarvestableComponent harvest;
            harvest.yieldItem = ItemId::Wood;
            harvest.health = 100.0f;
            harvest.maxHealth = 100.0f;
            harvest.yieldPerHit = 1;
            harvest.yieldOnDestroy = 5;
            registry.emplace<HarvestableComponent>(entity, harvest);
            registry.emplace<NameTag>(entity, "Tree");

            GridCoord gridCoord = PlacementGrid::WorldToGrid(candidate.position, 1.0f); // match your real cellSize
            placementGrid.Register(gridCoord, entity);

            chunk.treeEntities.push_back(entity);
        }

        chunk.isGenerated = true;

        m_LoadedChunks[result.coord] = chunk;
        {
            std::lock_guard<std::mutex> lock(m_RequestMutex);
            m_AwaitingProcessing.erase(result.coord); // NEW
        }
        
    }
}

// ChunkManager.cpp
ChunkManager::GeneratedVertexData ChunkManager::GenerateTerrainDataOnly(ChunkCoord coord, const TerrainSettings &settings) {
    float chunkOriginX = coord.x * CHUNK_WORLD_SIZE;
    float chunkOriginZ = coord.z * CHUNK_WORLD_SIZE;
    float cellSize = CHUNK_WORLD_SIZE / (CHUNK_VERTEX_RESOLUTION - 1);

    GeneratedVertexData data;

    int paddedRes = CHUNK_VERTEX_RESOLUTION + 2;
    std::vector<glm::vec3> paddedPositions(paddedRes * paddedRes);

    // NOTE: no DepletionMap here — the worker thread can't safely read live depletion state
    // (mining happens on the main thread concurrently). We use a temporary EMPTY DepletionMap
    // for generation, and re-apply real depletion coloring afterward on the main thread if needed.
    // For now, ore-tint coloring during initial generation will reflect "undepleted" state;
    // any already-mined patches near a freshly-generated chunk boundary would show full tint
    // until the next depletion-triggered regen — an acceptable simplification to start with.
    DepletionMap emptyDepletionMap;

    for (int z = 0; z < paddedRes; z++) {
        for (int x = 0; x < paddedRes; x++) {
            float worldX = chunkOriginX + (x - 1) * cellSize;
            float worldZ = chunkOriginZ + (z - 1) * cellSize;
            auto sample = TerrainGenerator::SampleTerrain(worldX, worldZ, settings, emptyDepletionMap);
            paddedPositions[z * paddedRes + x] = glm::vec3(worldX, sample.height, worldZ);
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

            float worldX = chunkOriginX + x * cellSize;
            float worldZ = chunkOriginZ + z * cellSize;
            auto sample = TerrainGenerator::SampleTerrain(worldX, worldZ, settings, emptyDepletionMap);

            Vertex &v = data.vertices[realIdx];
            v.position = paddedPositions[paddedIdx];
            v.normal = glm::normalize(paddedNormals[paddedIdx]);
            v.texCoord = glm::vec2((float)x / (CHUNK_VERTEX_RESOLUTION - 1), (float)z / (CHUNK_VERTEX_RESOLUTION - 1));
            v.color = sample.color;
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

std::vector<ChunkManager::TreeCandidate> ChunkManager::GenerateTreeCandidatesOnly(ChunkCoord coord, const TerrainSettings &settings) {
    std::vector<TreeCandidate> candidates;

    float chunkOriginX = coord.x * CHUNK_WORLD_SIZE;
    float chunkOriginZ = coord.z * CHUNK_WORLD_SIZE;
    float worldExtentZ = settings.gridDepth * settings.cellSize;

    FastNoiseLite treeJitterNoise;
    treeJitterNoise.SetSeed(settings.seed + 4000);
    treeJitterNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    treeJitterNoise.SetFrequency(0.4f);

    float sampleSpacing = 8.0f;

    for (float x = 0.0f; x < CHUNK_WORLD_SIZE; x += sampleSpacing) {
        for (float z = 0.0f; z < CHUNK_WORLD_SIZE; z += sampleSpacing) {
            float worldX = chunkOriginX + x;
            float worldZ = chunkOriginZ + z;

            BiomeId biome = BiomeMap::GetBiomeAt(worldX, worldZ, worldExtentZ, settings.seed);
            if (biome == BiomeId::Lake) continue;

            const BiomeDef &def = BiomeDatabase::Get(biome);
            if (def.treeDensity <= 0.0f) continue;

            auto deposit = OreDepositMap::GetDepositAt(worldX, worldZ, settings.seed);
            if (deposit) continue;

            float aridity = BiomeMap::GetAridity(worldX, worldZ, settings.seed);
            float wetness = 1.0f - aridity;
            float effectiveDensity = def.treeDensity * wetness;

            float roll = treeJitterNoise.GetNoise(worldX, worldZ);
            float threshold = 1.0f - (effectiveDensity * 2.0f);
            if (roll < threshold) continue;

            float jitterX = (rand() / (float)RAND_MAX - 0.5f) * sampleSpacing;
            float jitterZ = (rand() / (float)RAND_MAX - 0.5f) * sampleSpacing;
            float finalX = worldX + jitterX;
            float finalZ = worldZ + jitterZ;

            auto sample = TerrainGenerator::SampleHeight(finalX, finalZ, settings);

            candidates.push_back({ glm::vec3(finalX, sample, finalZ) });
        }
    }

    return candidates;
}