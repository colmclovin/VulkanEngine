#pragma once
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include "../Components/DamageType.h"

class DamageSystem {
public:
    // The only way damage should be applied. Later, this is where network commands and events hook in.
    static void ApplyDamage(entt::registry &registry, entt::entity target, float amount,
                            DamageType type = DamageType::Physical, entt::entity source = entt::null);
    // Handles anything at 0 health. Call once per frame, after combat.
    static void ProcessDeaths(entt::registry &registry, const glm::vec3 &worldSpawn);
};
