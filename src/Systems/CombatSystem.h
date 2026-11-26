#pragma once
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <vector>

class CombatSystem {
public:
    struct Shot {
        glm::vec3 from, to;
    };
    // Appends one entry per attack made this frame (used for the debug tracers)
    static void Update(entt::registry &registry, float dt, std::vector<Shot> &shots);
};