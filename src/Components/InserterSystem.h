// InserterSystem.h
#pragma once
#include "PlacementGrid.h"
#include <entt/entt.hpp>

class InserterSystem {
public:
    static void Update(entt::registry &registry, PlacementGrid &grid, float gridSize, float deltaTime);
};