// AnimationSystem.h
#pragma once
#include <entt/entt.hpp>

class AnimationSystem {
public:
    static void Update(entt::registry &registry, float deltaTime);
};