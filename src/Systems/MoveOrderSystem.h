#pragma once
#include <entt/entt.hpp>
#include "../Utils/GameSettings.h"

class MoveOrderSystem
{
public:
	static void Update(entt::registry& registry, float deltaTime, const TerrainSettings& settings);


};