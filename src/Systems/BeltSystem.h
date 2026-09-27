// BeltSystem.h
#pragma once
#include "../World/PlacementGrid.h"
#include <entt/entt.hpp>

class BeltSystem {
public:
	static void Update(entt::registry &registry, PlacementGrid &grid, float gridSize, float deltaTime);
};
