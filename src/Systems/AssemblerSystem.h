// AssemblerSystem.h
#pragma once
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>
#include "../Game/ItemDatabase.h"

class AssemblerSystem {
public:
    struct SpawnRequest {
        ItemId unit;
        glm::vec3 position;
        uint8_t team;
    };

	static void Update(entt::registry &registry, float deltaTime, std::vector<SpawnRequest> &spawns, int unitBudget);
};
