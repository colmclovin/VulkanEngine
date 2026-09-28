#pragma once
#include "ChunkCoord.h"
#include <unordered_map>
#include <unordered_set>

class RemovedTreesMap {
public:
    bool IsRemoved(ChunkCoord coord, int index) const {
        auto it = m_Removed.find(coord);
        if (it == m_Removed.end()) return false;
        return it->second.count(index) > 0;
    }

    void MarkRemoved(ChunkCoord coord, int index) {
        m_Removed[coord].insert(index);
    }

    // For serialization — expose the raw map
    const std::unordered_map<ChunkCoord, std::unordered_set<int>, ChunkCoordHash> &GetAll() const { return m_Removed; }
    void LoadFrom(std::unordered_map<ChunkCoord, std::unordered_set<int>, ChunkCoordHash> data) { m_Removed = std::move(data); }

private:
    std::unordered_map<ChunkCoord, std::unordered_set<int>, ChunkCoordHash> m_Removed;
};