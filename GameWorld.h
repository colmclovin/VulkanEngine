#pragma once
#include "EnttWrapper.h"
#include "Components.h"
#include <glm/glm.hpp>

class GameWorld {
public:
	GameWorld(int gridWidth, int gridHeight);

	// Entity creation
	entt::entity CreateBuilding(BuildingType type, int gridX, int gridY);
	entt::entity CreateItem(ItemType itemType, int gridX, int gridY);
	entt::entity CreateResourceNode(ItemType resourceType, int gridX, int gridY, int amount);

	// Entity queries
	entt::entity GetEntityAt(int gridX, int gridY);
	bool IsTileOccupied(int gridX, int gridY);
	bool CanPlaceBuilding(int gridX, int gridY);

	// Systems (update each frame)
	void UpdateConveyors(float deltaTime);
	void UpdateProduction(float deltaTime);
	void UpdateMining(float deltaTime);

	// Grid utilities
	glm::vec2 GridToWorld(int gridX, int gridY) const;
	glm::ivec2 WorldToGrid(const glm::vec2& worldPos) const;
	bool IsInBounds(int gridX, int gridY) const;

	// Access to registry
	entt::registry& GetRegistry() { return m_Registry; }
	const entt::registry& GetRegistry() const { return m_Registry; }

	// Grid info
	int GetGridWidth() const { return m_GridWidth; }
	int GetGridHeight() const { return m_GridHeight; }
	float GetTileSize() const { return m_TileSize; }

private:
	entt::registry m_Registry;

	int m_GridWidth;
	int m_GridHeight;
	float m_TileSize; // Size of one tile in pixels

	// Grid occupancy (entity at each grid position)
	std::vector<entt::entity> m_GridOccupancy;

	// Helper functions
	int GetGridIndex(int gridX, int gridY) const;
	void SetGridOccupancy(int gridX, int gridY, entt::entity entity);
	void ClearGridOccupancy(int gridX, int gridY);
};
