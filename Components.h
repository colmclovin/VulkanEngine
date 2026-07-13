#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

// Forward declarations
class Mesh;

// ============================================================================
// Core Transform Component
// ============================================================================
struct Transform {
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 rotation = glm::vec3(0.0f); // Euler angles in radians
	glm::vec3 scale = glm::vec3(1.0f);

	glm::mat4 GetModelMatrix() const {
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, position);
		model = glm::rotate(model, rotation.x, glm::vec3(1, 0, 0));
		model = glm::rotate(model, rotation.y, glm::vec3(0, 1, 0));
		model = glm::rotate(model, rotation.z, glm::vec3(0, 0, 1));
		model = glm::scale(model, scale);
		return model;
	}
};

// ============================================================================
// 3D Rendering Components
// ============================================================================

// Mesh component - references a shared mesh resource
struct MeshComponent {
	Mesh* mesh = nullptr;  // Pointer to shared mesh data (managed by ResourceManager)
	bool castsShadows = true;
};

// Material/rendering properties for 3D objects
struct MaterialComponent {
	glm::vec4 color = glm::vec4(1.0f);      // Base color/tint
	float metallic = 0.0f;                   // Metallic factor (0-1)
	float roughness = 0.5f;                  // Roughness factor (0-1)
	float ambient = 0.1f;                    // Ambient lighting
};

// ============================================================================
// 2D UI Components
// ============================================================================

// Simple 2D sprite for UI elements
struct SpriteComponent {
	glm::vec2 position = glm::vec2(0.0f);   // Screen position
	glm::vec2 size = glm::vec2(100.0f);     // Sprite size
	glm::vec4 color = glm::vec4(1.0f);      // Color tint
	float rotation = 0.0f;                   // Rotation in radians
	int layer = 0;                           // Render layer (higher = front)
};

// ============================================================================
// Game-Specific Components (Factorio-like)
// ============================================================================

// Grid position for building placement
struct GridPosition {
	int x = 0;
	int y = 0;

	bool operator==(const GridPosition& other) const {
		return x == other.x && y == other.y;
	}
};

// Building types in the factory
enum class BuildingType {
	None,
	Conveyor,
	Assembler,
	Furnace,
	Miner,
	Chest,
	Inserter,
	PowerPole,
	Count
};

// Building component for factory entities
struct BuildingComponent {
	BuildingType type = BuildingType::None;
	int rotation = 0;               // 0=North, 1=East, 2=South, 3=West
	bool isActive = true;           // Is the building powered/working
	float productionProgress = 0.0f; // For assemblers/furnaces
};

// Conveyor belt specific data
struct ConveyorComponent {
	glm::vec2 direction = glm::vec2(1.0f, 0.0f); // Belt direction
	float speed = 2.0f;                           // Items per second
};

// Production building (assembler/furnace)
struct ProductionComponent {
	std::string recipeID;           // What this building produces
	float productionTime = 5.0f;    // Seconds to produce one item
	float progress = 0.0f;          // Current production progress
	bool hasIngredients = false;    // Can it currently produce
};

// Inventory for chests and machines
struct InventoryComponent {
	static constexpr int MAX_SLOTS = 20;
	int usedSlots = 0;
};

// Mining drill specific data
struct MinerComponent {
	glm::vec2 miningPosition;       // Where it's mining from
	float miningSpeed = 1.0f;       // Resources per second
};

// Power network
struct PowerComponent {
	float powerGeneration = 0.0f;   // Watts generated (if generator)
	float powerConsumption = 0.0f;  // Watts consumed
	bool isPowered = false;         // Is connected to powered network
};

// ============================================================================
// Utility/Tag Components
// ============================================================================

// Simple name tag for debugging and identification
struct NameTag {
	std::string name;
};

// Marks an entity for deletion
struct DestroyTag {};

// Marks an entity as selected by the player
struct SelectedTag {};

// ============================================================================
// Camera Component (for camera entities)
// ============================================================================

struct CameraComponent {
	glm::mat4 viewMatrix = glm::mat4(1.0f);
	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	bool isPrimary = false;  // Is this the main rendering camera
};

// ============================================================================
// Helper Functions
// ============================================================================

namespace ComponentHelpers {
	// Convert grid position to world position
	inline glm::vec3 GridToWorld(const GridPosition& gridPos, float tileSize = 1.0f) {
		return glm::vec3(
			gridPos.x * tileSize,
			0.0f,
			gridPos.y * tileSize
		);
	}

	// Convert world position to grid position
	inline GridPosition WorldToGrid(const glm::vec3& worldPos, float tileSize = 1.0f) {
		return GridPosition{
			static_cast<int>(std::floor(worldPos.x / tileSize)),
			static_cast<int>(std::floor(worldPos.z / tileSize))
		};
	}

	// Get direction vector from rotation index (0-3)
	inline glm::vec2 GetDirectionFromRotation(int rotation) {
		switch (rotation % 4) {
			case 0: return glm::vec2(0.0f, 1.0f);  // North
			case 1: return glm::vec2(1.0f, 0.0f);  // East
			case 2: return glm::vec2(0.0f, -1.0f); // South
			case 3: return glm::vec2(-1.0f, 0.0f); // West
			default: return glm::vec2(0.0f, 1.0f);
		}
	}
}
