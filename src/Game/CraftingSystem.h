// CraftingSystem.h
#pragma once
#include <entt/entt.hpp>
#include "Recipe.h"

class CraftingSystem {
public:
    static bool CanCraft(entt::registry& registry, entt::entity actor, const Recipe& recipe);
    static bool TryCraft(entt::registry& registry, entt::entity actor, const Recipe& recipe);
};