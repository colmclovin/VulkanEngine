#pragma once
#include "../World/ResourceMap.h"
#include <entt/entt.hpp>

class MinerSystem {
public:
	static void Update(entt::registry &registry, ResourceMap &resourceMap, float deltaTime);
};
