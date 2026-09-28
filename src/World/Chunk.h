#pragma once
#include "../Rendering/Mesh.h"
#include "ResourceMap.h"
#include <entt/entt.hpp>
#include <memory>
#include <vector>
#include "ChunkCoord.h"

struct Chunk {
	ChunkCoord coord;
	std::shared_ptr<Mesh> terrainMesh;
	entt::entity terrainEntity = entt::null;
	std::vector<entt::entity> treeEntities; // so we can destroy/hide them when unloading
	bool isGenerated = false;
	glm::vec3 boundsCenter;
	float boundsRadius;
};
