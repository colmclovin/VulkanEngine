// ChunkManager.cpp
#include "ChunkManager.h"
#include "../Engine/VulkanEngine.h"
#include "../Components/Components.h"
#include "../Components/TerrainGenerator.h"
#include "../Components/WorldGenerator.h"
#include <iostream>
ChunkCoord ChunkManager::WorldToChunkCoord(glm::vec3 worldPos) {
    return {
        static_cast<int>(std::floor(worldPos.x / CHUNK_WORLD_SIZE)),
        static_cast<int>(std::floor(worldPos.z / CHUNK_WORLD_SIZE))
    };
}

void ChunkManager::Update(entt::registry &registry, glm::vec3 playerPosition, const TerrainSettings &settings,
                          VulkanEngine *engine, MeshRenderer *meshRenderer, int seed, DepletionMap &depletionMap, PlacementGrid &placementGrid) {
    ChunkCoord playerChunk = WorldToChunkCoord(playerPosition);

    // Determine which chunks SHOULD be loaded
    std::vector<ChunkCoord> desiredChunks;
    for (int dz = -LOAD_RADIUS_CHUNKS; dz <= LOAD_RADIUS_CHUNKS; dz++) {
        for (int dx = -LOAD_RADIUS_CHUNKS; dx <= LOAD_RADIUS_CHUNKS; dx++) {
            desiredChunks.push_back({ playerChunk.x + dx, playerChunk.z + dz });
        }
    }

    // Load any desired chunk not already loaded
    for (auto &coord : desiredChunks) {
        if (m_LoadedChunks.find(coord) == m_LoadedChunks.end()) {
            GenerateChunk(coord, registry, settings, engine, meshRenderer, seed, depletionMap, placementGrid);
        }
    }

    // Unload chunks that are no longer desired
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
    auto it = m_LoadedChunks.find(coord);
    if (it == m_LoadedChunks.end()) return;

    Chunk &chunk = it->second;
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