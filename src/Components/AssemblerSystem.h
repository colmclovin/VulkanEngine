// AssemblerSystem.h
#pragma once
#include <entt/entt.hpp>

class AssemblerSystem {
public:
    static void Update(entt::registry &registry, float deltaTime);
};