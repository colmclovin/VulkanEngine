#pragma once
#include <entt/entt.hpp>
#include <unordered_map>
#include <glm/glm.hpp>
#include <iostream>
#include <algorithm>   // for std::max_element/min_element

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
        std::cout << "Lookup (" << coord.x << "," << coord.z << ") -> " << (it != m_Occupancy.end() ? "FOUND" : "not found") << std::endl;
        return it != m_Occupancy.end() ? it->second : entt::null;

    }
    bool IsOccupied(GridCoord coord) const { return GetEntityAt(coord) != entt::null; }

    static std::vector<GridCoord> GetCoveredCells(glm::vec3 centerPos, glm::vec3 halfExtents, float gridSize) {
        std::vector<GridCoord> cells;

        // Convert half-extent to a whole tile radius, rounding to the nearest int rather than
        // reconstructing via floor(boundary + 0.5) which double-counts on exact tile boundaries.
        int tilesX = static_cast<int>(std::round((halfExtents.x * 2.0f) / gridSize));
        int tilesZ = static_cast<int>(std::round((halfExtents.z * 2.0f) / gridSize));
        tilesX = std::max(1, tilesX);
        tilesZ = std::max(1, tilesZ);

        GridCoord center = PlacementGrid::WorldToGrid(centerPos, gridSize);

        int halfX = tilesX / 2;
        int halfZ = tilesZ / 2;

        for (int x = -halfX; x < tilesX - halfX; x++) {
            for (int z = -halfZ; z < tilesZ - halfZ; z++) {
                cells.push_back({ center.x + x, center.z + z });
            }
        }
        return cells;
    }
    bool IsAreaOccupied(const std::vector<GridCoord> &cells) const {
        for (auto &c : cells) {
            if (IsOccupied(c)) return true;
        }
        return false;
    }

    void RegisterArea(const std::vector<GridCoord> &cells, entt::entity entity) {
        for (auto &c : cells) {
            Register(c, entity);
        }
    }

    void UnregisterArea(const std::vector<GridCoord> &cells) {
        for (auto &c : cells) {
            Unregister(c);
        }
    }


private:
    std::unordered_map<GridCoord, entt::entity, GridCoordHash> m_Occupancy;
};


    static entt::entity FindNeighborInDirection(entt::registry &registry, PlacementGrid &grid,
                                            glm::vec3 position, glm::vec3 halfExtents,
                                            glm::vec3 direction, float gridSize) {
    auto myCells = PlacementGrid::GetCoveredCells(position, halfExtents, gridSize);
    if (myCells.empty()) return entt::null;

    int dx = static_cast<int>(std::round(direction.x));
    int dz = static_cast<int>(std::round(direction.z));

    int edgeValue;
    if (dx != 0) {
        edgeValue = dx > 0 ? std::max_element(myCells.begin(), myCells.end(), [](auto &a, auto &b) { return a.x < b.x; })->x : std::min_element(myCells.begin(), myCells.end(), [](auto &a, auto &b) { return a.x < b.x; })->x;
    } else {
        edgeValue = dz > 0 ? std::max_element(myCells.begin(), myCells.end(), [](auto &a, auto &b) { return a.z < b.z; })->z : std::min_element(myCells.begin(), myCells.end(), [](auto &a, auto &b) { return a.z < b.z; })->z;
    }

    for (auto &cell : myCells) {
        bool onEdge = (dx != 0) ? (cell.x == edgeValue) : (cell.z == edgeValue);
        if (!onEdge) continue;

        GridCoord neighborCell = { cell.x + dx, cell.z + dz };
        entt::entity candidate = grid.GetEntityAt(neighborCell);
        if (registry.valid(candidate)) return candidate;
    }
    return entt::null;
}