// FogOfWar.cpp
#include "FogOfWar.h"
#include "../Components/Components.h"

void FogOfWar::Update(entt::registry& registry, uint8_t team) {


    size_t h = 1469598103u;
    {
        auto v = registry.view<TransformComponent, VisionComponent, TeamComponent>();
        for (auto e : v) {
            if (v.get<TeamComponent>(e).team != team) continue;
            glm::vec3 p = v.get<TransformComponent>(e).Position;
            h = h * 1099511628211u + (size_t)(int)std::floor(p.x / CELL);
            h = h * 1099511628211u + (size_t)(int)std::floor(p.z / CELL);
        }
    }
    if (h == m_LastHash) return; // nothing moved a cell, so the grid is unchanged
    m_LastHash = h;
    m_Dirty = true;

    // Demote last tick's visible cells to explored
    for (auto& [coord, chunk] : m_Chunks)
        for (auto& c : chunk.cells)
            if (c == Visible) c = Explored;

    auto view = registry.view<TransformComponent, VisionComponent, TeamComponent>();
    for (auto e : view) {
        if (view.get<TeamComponent>(e).team != team) continue;
        glm::vec3 p = view.get<TransformComponent>(e).Position;
        float r = view.get<VisionComponent>(e).radius;

        int minX = (int)std::floor((p.x - r) / CELL), maxX = (int)std::floor((p.x + r) / CELL);
        int minZ = (int)std::floor((p.z - r) / CELL), maxZ = (int)std::floor((p.z + r) / CELL);
        for (int cz = minZ; cz <= maxZ; cz++) {
            for (int cx = minX; cx <= maxX; cx++) {
                float dx = (cx + 0.5f) * CELL - p.x, dz = (cz + 0.5f) * CELL - p.z;
                if (dx * dx + dz * dz > r * r) continue;

                ChunkCoord cc{ (int)std::floor((float)cx / CELLS_PER_CHUNK), (int)std::floor((float)cz / CELLS_PER_CHUNK) };
                int lx = cx - cc.x * CELLS_PER_CHUNK, lz = cz - cc.z * CELLS_PER_CHUNK;
                m_Chunks[cc].cells[lz * CELLS_PER_CHUNK + lx] = Visible;
            }
        }
    }
}

uint8_t FogOfWar::GetState(float worldX, float worldZ) const {
    int cx = (int)std::floor(worldX / CELL), cz = (int)std::floor(worldZ / CELL);
    ChunkCoord cc{ (int)std::floor((float)cx / CELLS_PER_CHUNK), (int)std::floor((float)cz / CELLS_PER_CHUNK) };
    auto it = m_Chunks.find(cc);
    if (it == m_Chunks.end()) return Unexplored;
    int lx = cx - cc.x * CELLS_PER_CHUNK, lz = cz - cc.z * CELLS_PER_CHUNK;
    return it->second.cells[lz * CELLS_PER_CHUNK + lx];
}
void FogOfWar::FillWindow(uint8_t *out, float originX, float originZ, int size) const {
    memset(out, 0, size * size);
    int ox = (int)std::lround(originX / CELL);
    int oz = (int)std::lround(originZ / CELL);

    for (const auto &[cc, chunk] : m_Chunks) {
        int bx = cc.x * CELLS_PER_CHUNK - ox;
        int bz = cc.z * CELLS_PER_CHUNK - oz;
        if (bx + CELLS_PER_CHUNK <= 0 || bz + CELLS_PER_CHUNK <= 0 || bx >= size || bz >= size) continue;

        for (int lz = 0; lz < CELLS_PER_CHUNK; lz++) {
            int z = bz + lz;
            if (z < 0 || z >= size) continue;
            for (int lx = 0; lx < CELLS_PER_CHUNK; lx++) {
                int x = bx + lx;
                if (x < 0 || x >= size) continue;
                uint8_t s = chunk.cells[lz * CELLS_PER_CHUNK + lx];
                out[z * size + x] = (s == Visible) ? 255 : (s == Explored ? 128 : 0);
            }
        }
    }
}