#pragma once
#include <entt/entt.hpp>
#include <unordered_map>
#include <glm/glm.hpp>

struct GridCoord {
    int x, z;
    bool operator==(const GridCoord &o) const { return x == o.x && z == o.z; }
};

struct GridCoordHash {
    size_t operator()(const GridCoord &c) const {
        return std::hash<int>()(c.x) * 73856093u ^ std::hash<int>()(c.z) * 19349663u;
    }
};

class PlacementGrid {
public:
    static GridCoord WorldToGrid(glm::vec3 pos, float gridSize) {
        return { static_cast<int>(std::round(pos.x / gridSize)), static_cast<int>(std::round(pos.z / gridSize)) };
    }

    void Register(GridCoord coord, entt::entity entity) { m_Occupancy[coord] = entity; }
    void Unregister(GridCoord coord) { m_Occupancy.erase(coord); }

    entt::entity GetEntityAt(GridCoord coord) const {
        auto it = m_Occupancy.find(coord);
        return it != m_Occupancy.end() ? it->second : entt::null;
    }

private:
    std::unordered_map<GridCoord, entt::entity, GridCoordHash> m_Occupancy;
};