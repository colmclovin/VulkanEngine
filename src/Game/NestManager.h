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

// Enemy nests. Dormant until the player places their first Catalyst, which starts the invasion.
class NestManager {
public:
    static constexpr int FIRST_WAVE_NEST_COUNT = 4;

    // Call every frame: starts the invasion once a player Catalyst exists, then ticks the nest economy.
    void Update(entt::registry &registry, int seed, const FogOfWar &fog, float dt, double now);

    // Picks starting nests around the Catalyst, outside anything the player has seen
    void Activate(glm::vec2 catalystPos, int seed, const FogOfWar &fog, int count, double now);
    void Tick(float dt, double now);
    void Clear() { *this = NestManager{}; }

    bool InvasionStarted() const { return m_InvasionStarted; }
    double InvasionStartTime() const { return m_InvasionStartTime; }
    glm::vec2 CatalystPos() const { return m_CatalystPos; }
    const std::vector<NestState> &Nests() const { return m_Nests; }
    std::vector<NestState> &Nests() { return m_Nests; }

private:
    std::vector<NestState> m_Nests;
    float m_Accum = 0.0f;
    bool m_InvasionStarted = false;
    double m_InvasionStartTime = 0.0;
    glm::vec2 m_CatalystPos{};
};
