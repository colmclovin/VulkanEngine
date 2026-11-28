// NestManager.h
#pragma once
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <vector>

class FogOfWar;

struct NestState {
    int cellX, cellZ;
    glm::vec2 pos;
    int tier;
    float stockpile = 0.0f; // single abstract resource for now
    float strength = 0.0f; // accumulated power from built units
    double nextDecisionTime = 0.0;
    bool materialized = false;
};

class NestManager {
public:
    // Picks starting nests around the Catalyst, outside anything the player has seen
    void Activate(glm::vec2 catalystPos, int seed, const FogOfWar &fog, int count, double now);
    void Tick(float dt, double now);
    void Clear() { m_Nests.clear(); }
    const std::vector<NestState> &Nests() const { return m_Nests; }
    std::vector<NestState> &Nests() { return m_Nests; }

private:
    std::vector<NestState> m_Nests;
    float m_Accum = 0.0f;
};