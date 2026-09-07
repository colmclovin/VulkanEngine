// FurnaceSystem.h
#pragma once
#include <entt/entt.hpp>

class FurnaceSystem {
public:
    static void Update(entt::registry &registry, float deltaTime);
};