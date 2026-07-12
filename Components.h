#pragma once
#include <glm/glm.hpp>
#include <string>
#include <map>

// Position in the world grid (tile-based)
struct GridPosition {
	int x = 0;
	int y = 0;
};

// Transform in world space (pixels)
struct Transform {
	glm::vec2 position = { 0.0f, 0.0f };
	glm::vec2 scale = { 1.0f, 1.0f };
	float rotation = 0.0f; // in degrees
};

// Visual representation
struct SpriteComponent {
	glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
	uint32_t textureID = 0; // 0 = white square (no texture)
	int layer = 0; // rendering order (higher = drawn on top)
};

// Building types for the factory
enum class BuildingType {
	None,
	Conveyor,
	Inserter,
	Assembler,
	Furnace,
	Miner,
	Chest
};

// Building component
struct BuildingComponent {
	BuildingType type = BuildingType::None;
	int rotation = 0; // 0, 90, 180, 270 degrees
	bool isActive = true;
};

// Item types in the game
enum class ItemType {
	None,
	IronOre,
	CopperOre,
	IronPlate,
	CopperPlate,
	IronGear,
	ElectronicCircuit
};

// Item being transported or stored
struct ItemComponent {
	ItemType type = ItemType::None;
	float progress = 0.0f; // 0-1 progress along conveyor
	int stackSize = 1;
};

// Inventory for buildings
struct InventoryComponent {
	std::map<ItemType, int> items; // itemType -> count
	int maxSlots = 10;
	int maxStackSize = 100;
};

// Conveyor belt component
struct ConveyorComponent {
	glm::vec2 direction = { 1.0f, 0.0f }; // normalized direction
	float speed = 1.0f; // tiles per second
};

// Production building (assembler, furnace)
struct ProductionComponent {
	ItemType inputType = ItemType::None;
	ItemType outputType = ItemType::None;
	float productionTime = 1.0f; // seconds to craft
	float progress = 0.0f; // current crafting progress
	bool isCrafting = false;
};

// Resource node (ore patch)
struct ResourceComponent {
	ItemType resourceType = ItemType::IronOre;
	int amount = 1000; // remaining resources
	float miningSpeed = 1.0f; // items per second
};

// Tag components (empty structs for filtering)
struct PlayerControlledTag {};
struct SelectableTag {};
struct CollidableTag {};
