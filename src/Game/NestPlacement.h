// NestPlacement.h
#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <optional>

struct NestSite {
    int cellX, cellZ;
    glm::vec2 pos;
    int distanceTier; // 0..4
};

class NestPlacement {
public:
    static constexpr float CELL = 160.0f; // one possible nest per cell
    static constexpr float SAFE_RADIUS = 250.0f; // no nests this close to the origin

    static std::optional<NestSite> GetNestInCell(int cx, int cz, int seed, glm::vec2 origin);
};