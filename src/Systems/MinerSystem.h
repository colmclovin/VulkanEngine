#pragma once
#include "../Game/OreDepositMap.h"
#include "../World/DepletionMap.h"
#include <entt/entt.hpp>
#include "../Utils/GameSettings.h"
#include "../Game/ChunkManager.h"

class MinerSystem {
public:
	static void Update(entt::registry& registry, DepletionMap& depletionMap, const TerrainSettings& terrainSettings, float deltaTime, ChunkManager& m_ChunkManager);
};
