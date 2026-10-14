#pragma once
#include <glm/glm.hpp>
#include "../World/ChunkCoord.h"
#include <entt/entt.hpp>


class FogOfWar {
public:
    static constexpr float CELL = 4.0f;
    static constexpr int CELLS_PER_CHUNK = 8;   // CHUNK_WORLD_SIZE / CELL

    enum : uint8_t { Unexplored = 0, Explored = 1, Visible = 2 };

    void Update(entt::registry& registry, uint8_t team);
    uint8_t GetState(float worldX, float worldZ) const;
    void FillWindow(uint8_t *out, float originX, float originZ, int size) const;
    bool ConsumeDirty() {
        bool d = m_Dirty;
        m_Dirty = false;
        return d;
    }


private:
    struct FogChunk { uint8_t cells[CELLS_PER_CHUNK * CELLS_PER_CHUNK] = {}; };
    std::unordered_map<ChunkCoord, FogChunk, ChunkCoordHash> m_Chunks;

        bool m_Dirty = true;
    size_t m_LastHash = 0;
};