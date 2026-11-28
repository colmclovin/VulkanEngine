// NestPlacement.cpp
#include "NestPlacement.h"
#include "../Utils/GpuNoiseMatch.h"
#include <algorithm>
#include <cmath>

static uint32_t Hash3(int x, int z, uint32_t seed, uint32_t salt) {
    uint32_t h = seed * 0x9E3779B1u ^ (uint32_t)x * 0x85EBCA6Bu ^ (uint32_t)z * 0xC2B2AE35u ^ salt * 0x27D4EB2Fu;
    h ^= h >> 15;
    h *= 0x2C1B3C6Du;
    h ^= h >> 12;
    h *= 0x297A2D39u;
    h ^= h >> 15;
    return h;
}
static float Unit(uint32_t h) {
    return (h & 0xFFFFFF) / float(0x1000000);
} // 0..1

std::optional<NestSite> NestPlacement::GetNestInCell(int cx, int cz, int seed, glm::vec2 origin) {
    uint32_t s = (uint32_t)seed;
    if (Unit(Hash3(cx, cz, s, 1)) > 0.35f) return std::nullopt; // about 35% of cells hold a nest

    glm::vec2 pos((cx + 0.15f + 0.7f * Unit(Hash3(cx, cz, s, 2))) * CELL,
                  (cz + 0.15f + 0.7f * Unit(Hash3(cx, cz, s, 3))) * CELL);

   float dist = glm::length(pos - origin); // was length(pos)
    if (dist < SAFE_RADIUS) return std::nullopt;


    int biome = GpuNoiseMatch::DetermineBiome(pos.x, pos.y, (float)seed);
    if (biome == BIOME_LAKE) return std::nullopt;

   int tier = std::clamp((int)((dist - SAFE_RADIUS) / 600.0f), 0, 4);
    return NestSite{ cx, cz, pos, tier };
}