// ChunkManager.cpp
#include "ChunkManager.h"
#include <unordered_set>
#include "ChunkGenerator.h"
#include "../Renderer/TerrainRenderer.h"
#include "../Renderer/TreeRenderer.h"
#include "../Utils/GpuNoiseMatch.h"

// CPU height sampling assumes this vertex spacing; change both together
static_assert(ChunkManager::MESH_CELL_SIZE == GpuNoiseMatch::TERRAIN_MESH_CELL_SIZE,
              "GpuNoiseMatch::TERRAIN_MESH_CELL_SIZE must match the chunk mesh spacing");

namespace {
    constexpr float BASELINE_ISO_DISTANCE = 20.0f; // camera distance at which the load radius is 1x
    constexpr float BEHIND_RADIUS_FRACTION = 0.35f; // chunks behind the camera load out to 35% of the radius
    constexpr size_t MAX_DEPLETION_ENTRIES = 4;    // must match depletionEntries[] in chunk_generate_pass1.comp
    constexpr float CHUNK_BOUNDS_RADIUS = ChunkManager::CHUNK_WORLD_SIZE * 0.75f;

    // Chunks within a radius of center that's full-size in front of the camera and shrinks behind it,
    // all scaled up by zoomFactor.
    std::vector<ChunkCoord> ComputeDesiredChunks(ChunkCoord centerChunk, glm::vec3 cameraForward, float zoomFactor) {
        glm::vec2 forward2D = glm::normalize(glm::vec2(cameraForward.x, cameraForward.z));
        const float radius = static_cast<float>(ChunkManager::LOAD_RADIUS_CHUNKS);
        const int loopRadius = static_cast<int>(radius * zoomFactor);

        std::vector<ChunkCoord> desired;
        for (int dz = -loopRadius; dz <= loopRadius; dz++) {
            for (int dx = -loopRadius; dx <= loopRadius; dx++) {
                float distance = glm::length(glm::vec2(dx, dz));
                bool include = distance < 0.001f;
                if (!include) {
                    float facing = glm::dot(glm::vec2(dx, dz) / distance, forward2D); // -1 behind .. +1 ahead
                    float effectiveRadius = glm::mix(radius * BEHIND_RADIUS_FRACTION, radius, (facing + 1.0f) * 0.5f);
                    include = distance <= effectiveRadius * zoomFactor;
                }
                if (include) desired.push_back({ centerChunk.x + dx, centerChunk.z + dz });
            }
        }
        return desired;
    }

    glm::vec3 ChunkBoundsCenter(ChunkCoord coord) {
        return glm::vec3((coord.x + 0.5f) * ChunkManager::CHUNK_WORLD_SIZE, 0.0f, (coord.z + 0.5f) * ChunkManager::CHUNK_WORLD_SIZE);
    }
}

ChunkCoord ChunkManager::WorldToChunkCoord(glm::vec3 worldPos) {
    return {
        static_cast<int>(std::floor(worldPos.x / CHUNK_WORLD_SIZE)),
        static_cast<int>(std::floor(worldPos.z / CHUNK_WORLD_SIZE))
    };
}

void ChunkManager::Update(glm::vec3 center, glm::vec3 cameraForward, float isoDistance, TerrainRenderer *terrainRenderer,
                          TreeRenderer *treeRenderer) {
    float zoomFactor = glm::clamp(isoDistance / BASELINE_ISO_DISTANCE, 1.0f, MAX_ZOOM_FACTOR);
    std::vector<ChunkCoord> desired = ComputeDesiredChunks(WorldToChunkCoord(center), cameraForward, zoomFactor);

    // Allocate GPU slots for newly desired chunks and queue them for generation
    for (auto &coord : desired) {
        if (m_LoadedChunks.count(coord)) continue;

        auto terrainSlot = terrainRenderer->AllocateChunkSlot();
        if (!terrainSlot) continue; // pool full this frame, try again next frame

        auto treeSlot = treeRenderer->AllocateChunkSlot();
        if (!treeSlot) {
            terrainRenderer->FreeChunkSlot(*terrainSlot); // keep terrain and tree slots paired
            continue;
        }

        Chunk chunk;
        chunk.coord = coord;
        chunk.terrainSlot = *terrainSlot;
        chunk.treeSlot = *treeSlot;
        m_LoadedChunks[coord] = chunk;
        m_PendingGpuGeneration.push_back(coord);
    }

    // Unload chunks that are no longer desired
    std::unordered_set<ChunkCoord, ChunkCoordHash> desiredSet(desired.begin(), desired.end());
    std::vector<ChunkCoord> toUnload;
    for (auto &[coord, chunk] : m_LoadedChunks)
        if (!desiredSet.count(coord)) toUnload.push_back(coord);
    for (auto &coord : toUnload) UnloadChunk(coord, terrainRenderer, treeRenderer);
}

void ChunkManager::UnloadChunk(ChunkCoord coord, TerrainRenderer *terrainRenderer, TreeRenderer *treeRenderer) {
    auto it = m_LoadedChunks.find(coord);
    if (it == m_LoadedChunks.end()) return;

    if (it->second.terrainSlot != UINT32_MAX) terrainRenderer->FreeChunkSlot(it->second.terrainSlot);
    if (it->second.treeSlot != UINT32_MAX) treeRenderer->FreeChunkSlot(it->second.treeSlot);
    m_LoadedChunks.erase(it);
}

void ChunkManager::UnloadAll(TerrainRenderer *terrainRenderer, TreeRenderer *treeRenderer) {
    std::vector<ChunkCoord> coords;
    coords.reserve(m_LoadedChunks.size());
    for (auto &[coord, chunk] : m_LoadedChunks) coords.push_back(coord);
    for (auto &coord : coords) UnloadChunk(coord, terrainRenderer, treeRenderer);

    m_LoadedChunks.clear();
    m_PendingGpuGeneration.clear();
}

void ChunkManager::BeginInitialLoad(glm::vec3 playerPos, glm::vec3 cameraForward) {
    // Same shape Update() loads at normal zoom, so the loading screen finishes once those chunks exist
    m_InitialLoadTarget = ComputeDesiredChunks(WorldToChunkCoord(playerPos), cameraForward, 1.0f).size();
}

bool ChunkManager::IsInitialLoadComplete() const {
    return m_LoadedChunks.size() >= m_InitialLoadTarget;
}

float ChunkManager::GetInitialLoadProgress() const {
    if (m_InitialLoadTarget == 0) return 1.0f;
    return glm::clamp((float)m_LoadedChunks.size() / (float)m_InitialLoadTarget, 0.0f, 1.0f);
}

void ChunkManager::RecordPendingGeneration(VkCommandBuffer commandBuffer, ChunkGenerator *generator,
                                           TerrainRenderer *terrainRenderer, TreeRenderer *treeRenderer,
                                           const TerrainSettings &settings, const DepletionMap &depletionMap,
                                           const RemovedTreesMap &removedTreesMap) {
    for (auto &coord : m_PendingGpuGeneration) {
        auto it = m_LoadedChunks.find(coord);
        if (it == m_LoadedChunks.end() || it->second.isGenerated) continue;

        float chunkOriginX = coord.x * CHUNK_WORLD_SIZE;
        float chunkOriginZ = coord.z * CHUNK_WORLD_SIZE;

        // Depleted ore cells inside this chunk, so the shader can fade their color
        std::vector<glm::vec4> depletionEntries;
        for (auto &[gridCoord, fraction] : depletionMap.GetAll()) {
            float wx = static_cast<float>(gridCoord.x);
            float wz = static_cast<float>(gridCoord.z);
            if (wx >= chunkOriginX && wx < chunkOriginX + CHUNK_WORLD_SIZE && wz >= chunkOriginZ && wz < chunkOriginZ + CHUNK_WORLD_SIZE) {
                depletionEntries.push_back(glm::vec4(wx, wz, fraction, 0.0f));
                if (depletionEntries.size() >= MAX_DEPLETION_ENTRIES) break;
            }
        }

        // Trees chopped down in this chunk, so the shader skips them
        auto removedIt = removedTreesMap.GetAll().find(coord);
        auto removedIndices = removedIt != removedTreesMap.GetAll().end() ? removedIt->second : std::unordered_set<int>{};

        generator->RecordGenerateChunk(commandBuffer, it->second.terrainSlot, it->second.treeSlot, chunkOriginX, chunkOriginZ,
                                       MESH_CELL_SIZE, (float)settings.seed, depletionEntries, removedIndices);

        terrainRenderer->UploadChunkBounds(it->second.terrainSlot, ChunkBoundsCenter(coord), CHUNK_BOUNDS_RADIUS);
        treeRenderer->UploadChunkBounds(it->second.treeSlot, ChunkBoundsCenter(coord), CHUNK_BOUNDS_RADIUS);
        it->second.isGenerated = true;
    }
    m_PendingGpuGeneration.clear();
}

void ChunkManager::RequestChunkRegeneration(ChunkCoord coord) {
    auto it = m_LoadedChunks.find(coord);
    if (it == m_LoadedChunks.end()) return; // not loaded; it'll generate with current data when it loads

    it->second.isGenerated = false;
    m_PendingGpuGeneration.push_back(coord);
}

std::optional<uint32_t> ChunkManager::GetTreeSlotForChunk(ChunkCoord coord) const {
    auto it = m_LoadedChunks.find(coord);
    if (it == m_LoadedChunks.end() || !it->second.isGenerated) return std::nullopt;
    return it->second.treeSlot;
}
