#pragma once
#include <entt/entt.hpp>
#include <glm/glm.hpp>

class PlayerAnimationSystem {
public:
    static void Update(entt::registry &registry, entt::entity player, glm::vec3 velocity);
};