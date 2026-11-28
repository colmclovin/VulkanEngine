#pragma once
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>
#include <glm/glm.hpp>
#include <entt/entt.hpp>
#include "../World/ChunkCoord.h"

// Per-team fog of war on a CELL-sized grid, stored sparsely in chunks of CELLS_PER_CHUNK x CELLS_PER_CHUNK cells.
class FogOfWar {
public:
    static constexpr float CELL = 4.0f;
    static constexpr int CELLS_PER_CHUNK = 8;   // CHUNK_WORLD_SIZE / CELL
    static_assert(CELLS_PER_CHUNK * CELLS_PER_CHUNK == 64, "explored masks are packed into a uint64_t per chunk");

    enum : uint8_t { Unexplored = 0, Explored = 1, Visible = 2 };

    void Update(entt::registry& registry, uint8_t team);
    uint8_t GetState(float worldX, float worldZ) const;
    void FillWindow(uint8_t *out, float originX, float originZ, int size) const;
    bool ConsumeDirty() {
        bool d = m_Dirty;
        m_Dirty = false;
        return d;
    }

    // --- Saving: one bit per cell (bit index = localZ * CELLS_PER_CHUNK + localX), set if explored or visible.
    // Visible isn't saved separately; it's recomputed from units on the next Update.
    std::vector<std::pair<ChunkCoord, uint64_t>> GetExploredMasks() const;
    void SetExploredMask(ChunkCoord chunk, uint64_t mask);

private:
    struct FogChunk { uint8_t cells[CELLS_PER_CHUNK * CELLS_PER_CHUNK] = {}; };
    std::unordered_map<ChunkCoord, FogChunk, ChunkCoordHash> m_Chunks;

    bool m_Dirty = true;
    size_t m_LastHash = 0;
};