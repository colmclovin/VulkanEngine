// TreeHealthMap.h
#pragma once
#include <unordered_map>
#include "ChunkCoord.h"

class TreeHealthMap {
public:
    float GetHealth(ChunkCoord coord, int candidateIndex, float maxHealth) const {
        auto it = m_Health.find({ coord, candidateIndex });
        return it != m_Health.end() ? it->second : maxHealth;   // undamaged trees default to full health
    }

    void Damage(ChunkCoord coord, int candidateIndex, float amount, float maxHealth) {
        float current = GetHealth(coord, candidateIndex, maxHealth);
        m_Health[{ coord, candidateIndex }] = current - amount;
    }

    bool IsDead(ChunkCoord coord, int candidateIndex, float maxHealth) const {
        return GetHealth(coord, candidateIndex, maxHealth) <= 0.0f;
    }

    void Clear(ChunkCoord coord, int candidateIndex) {
        m_Health.erase({ coord, candidateIndex });
    }

    // NLOHMANN_DEFINE_TYPE_INTRUSIVE-style serialization, matching your other persistent maps, if you want this saved
    // (worth doing, so partially-chopped trees don't reset to full health on reload)

private:
    struct Key {
        ChunkCoord coord;
        int index;
        bool operator==(const Key& other) const { return coord == other.coord && index == other.index; }
    };
    struct KeyHash {
        size_t operator()(const Key& k) const {
            return std::hash<int>()(k.coord.x) ^ (std::hash<int>()(k.coord.z) << 1) ^ (std::hash<int>()(k.index) << 2);
        }
    };
    std::unordered_map<Key, float, KeyHash> m_Health;
};