#include "GameWorld.h"
#include <glm/gtc/constants.hpp>

GameWorld::GameWorld(int gridWidth, int gridHeight)
	: m_GridWidth(gridWidth)
	, m_GridHeight(gridHeight)
	, m_TileSize(32.0f) // 32 pixels per tile
{
	// Initialize grid occupancy
	m_GridOccupancy.resize(gridWidth * gridHeight, entt::null);
}

entt::entity GameWorld::CreateBuilding(BuildingType type, int gridX, int gridY) {
	if (!CanPlaceBuilding(gridX, gridY)) {
		return entt::null;
	}

	auto entity = m_Registry.create();

	// Add grid position
	GridPosition gridPos;
	gridPos.x = gridX;
	gridPos.y = gridY;
	m_Registry.emplace<GridPosition>(entity, gridPos);

	// Add transform (world position)
	glm::vec2 worldPos = GridToWorld(gridX, gridY);
	Transform transform;
	transform.position = worldPos;
	transform.scale = glm::vec2(m_TileSize, m_TileSize);
	transform.rotation = 0.0f;
	m_Registry.emplace<Transform>(entity, transform);

	// Add building component
	BuildingComponent building;
	building.type = type;
	building.rotation = 0;
	building.isActive = true;
	m_Registry.emplace<BuildingComponent>(entity, building);

	// Add sprite
	glm::vec4 color = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
	switch (type) {
		case BuildingType::Conveyor:
			color = glm::vec4(0.9f, 0.7f, 0.3f, 1.0f); // Yellow
			{
				ConveyorComponent conveyor;
				conveyor.direction = glm::vec2(1.0f, 0.0f);
				conveyor.speed = 2.0f;
				m_Registry.emplace<ConveyorComponent>(entity, conveyor);
			}
			break;
		case BuildingType::Assembler:
			color = glm::vec4(0.3f, 0.5f, 0.8f, 1.0f); // Blue
			m_Registry.emplace<ProductionComponent>(entity);
			m_Registry.emplace<InventoryComponent>(entity);
			break;
		case BuildingType::Furnace:
			color = glm::vec4(0.8f, 0.3f, 0.2f, 1.0f); // Red
			m_Registry.emplace<ProductionComponent>(entity);
			m_Registry.emplace<InventoryComponent>(entity);
			break;
		case BuildingType::Miner:
			color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray
			m_Registry.emplace<InventoryComponent>(entity);
			break;
		case BuildingType::Chest:
			color = glm::vec4(0.6f, 0.4f, 0.2f, 1.0f); // Brown
			m_Registry.emplace<InventoryComponent>(entity);
			break;
	}

	SpriteComponent sprite;
	sprite.color = color;
	sprite.textureID = 0;
	sprite.layer = 1;
	m_Registry.emplace<SpriteComponent>(entity, sprite);

	// Mark grid as occupied
	SetGridOccupancy(gridX, gridY, entity);

	return entity;
}

entt::entity GameWorld::CreateItem(ItemType itemType, int gridX, int gridY) {
	auto entity = m_Registry.create();

	GridPosition gridPos;
	gridPos.x = gridX;
	gridPos.y = gridY;
	m_Registry.emplace<GridPosition>(entity, gridPos);

	glm::vec2 worldPos = GridToWorld(gridX, gridY);
	Transform transform;
	transform.position = worldPos;
	transform.scale = glm::vec2(16.0f, 16.0f);
	transform.rotation = 0.0f;
	m_Registry.emplace<Transform>(entity, transform);

	ItemComponent item;
	item.type = itemType;
	item.progress = 0.0f;
	item.stackSize = 1;
	m_Registry.emplace<ItemComponent>(entity, item);

	// Color based on item type
	glm::vec4 color;
	switch (itemType) {
		case ItemType::IronOre:
			color = glm::vec4(0.6f, 0.6f, 0.7f, 1.0f);
			break;
		case ItemType::CopperOre:
			color = glm::vec4(0.8f, 0.5f, 0.3f, 1.0f);
			break;
		case ItemType::IronPlate:
			color = glm::vec4(0.7f, 0.7f, 0.8f, 1.0f);
			break;
		case ItemType::CopperPlate:
			color = glm::vec4(0.9f, 0.6f, 0.4f, 1.0f);
			break;
		default:
			color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			break;
	}

	m_Registry.emplace<SpriteComponent>(entity, color, 0u, 2);

	return entity;
}

entt::entity GameWorld::CreateResourceNode(ItemType resourceType, int gridX, int gridY, int amount) {
	if (!CanPlaceBuilding(gridX, gridY)) {
		return entt::null;
	}

	auto entity = m_Registry.create();

	GridPosition gridPos;
	gridPos.x = gridX;
	gridPos.y = gridY;
	m_Registry.emplace<GridPosition>(entity, gridPos);

	glm::vec2 worldPos = GridToWorld(gridX, gridY);
	Transform transform;
	transform.position = worldPos;
	transform.scale = glm::vec2(m_TileSize, m_TileSize);
	transform.rotation = 0.0f;
	m_Registry.emplace<Transform>(entity, transform);

	ResourceComponent resource;
	resource.resourceType = resourceType;
	resource.amount = amount;
	resource.miningSpeed = 1.0f;
	m_Registry.emplace<ResourceComponent>(entity, resource);

	glm::vec4 color = (resourceType == ItemType::IronOre) 
		? glm::vec4(0.4f, 0.4f, 0.5f, 1.0f) 
		: glm::vec4(0.6f, 0.3f, 0.1f, 1.0f);

	SpriteComponent sprite;
	sprite.color = color;
	sprite.textureID = 0;
	sprite.layer = 0;
	m_Registry.emplace<SpriteComponent>(entity, sprite);

	SetGridOccupancy(gridX, gridY, entity);

	return entity;
}

entt::entity GameWorld::GetEntityAt(int gridX, int gridY) {
	if (!IsInBounds(gridX, gridY)) {
		return entt::null;
	}
	return m_GridOccupancy[GetGridIndex(gridX, gridY)];
}

bool GameWorld::IsTileOccupied(int gridX, int gridY) {
	return GetEntityAt(gridX, gridY) != entt::null;
}

bool GameWorld::CanPlaceBuilding(int gridX, int gridY) {
	return IsInBounds(gridX, gridY) && !IsTileOccupied(gridX, gridY);
}

void GameWorld::UpdateConveyors(float deltaTime) {
	auto view = m_Registry.view<ConveyorComponent, Transform, GridPosition>();

	for (auto entity : view) {
		auto& conveyor = view.get<ConveyorComponent>(entity);
		// TODO: Move items on conveyor
	}
}

void GameWorld::UpdateProduction(float deltaTime) {
	auto view = m_Registry.view<ProductionComponent, InventoryComponent>();

	for (auto entity : view) {
		auto& production = view.get<ProductionComponent>(entity);
		auto& inventory = view.get<InventoryComponent>(entity);

		if (production.isCrafting) {
			production.progress += deltaTime;

			if (production.progress >= production.productionTime) {
				// Craft complete
				inventory.items[production.outputType]++;
				production.progress = 0.0f;
				production.isCrafting = false;
			}
		} else {
			// Check if can start crafting
			if (inventory.items[production.inputType] > 0) {
				inventory.items[production.inputType]--;
				production.isCrafting = true;
				production.progress = 0.0f;
			}
		}
	}
}

void GameWorld::UpdateMining(float deltaTime) {
	auto view = m_Registry.view<ResourceComponent, GridPosition>();

	for (auto entity : view) {
		auto& resource = view.get<ResourceComponent>(entity);
		// TODO: Check for miners nearby and extract resources
	}
}

glm::vec2 GameWorld::GridToWorld(int gridX, int gridY) const {
	return glm::vec2(gridX * m_TileSize, gridY * m_TileSize);
}

glm::ivec2 GameWorld::WorldToGrid(const glm::vec2& worldPos) const {
	return glm::ivec2(
		static_cast<int>(worldPos.x / m_TileSize),
		static_cast<int>(worldPos.y / m_TileSize)
	);
}

bool GameWorld::IsInBounds(int gridX, int gridY) const {
	return gridX >= 0 && gridX < m_GridWidth && gridY >= 0 && gridY < m_GridHeight;
}

int GameWorld::GetGridIndex(int gridX, int gridY) const {
	return gridY * m_GridWidth + gridX;
}

void GameWorld::SetGridOccupancy(int gridX, int gridY, entt::entity entity) {
	if (IsInBounds(gridX, gridY)) {
		m_GridOccupancy[GetGridIndex(gridX, gridY)] = entity;
	}
}

void GameWorld::ClearGridOccupancy(int gridX, int gridY) {
	if (IsInBounds(gridX, gridY)) {
		m_GridOccupancy[GetGridIndex(gridX, gridY)] = entt::null;
	}
}
