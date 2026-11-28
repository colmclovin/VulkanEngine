#pragma once
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <memory>
#include "ItemDatabase.h"
#include "../Components/TransformComponent.h"

class SkinnedMesh;

// Builds the component sets for player and unit entities, so new-game, loading and
// unit production all create identical entities.
namespace EntityFactory {
    constexpr const char *UNIT_MODEL_PATH = "Assets/Models/Test1.glb";
    constexpr glm::vec3 HUMANOID_HALF_EXTENTS{ 0.5f, 1.0f, 0.5f };
    constexpr float PLAYER_VISION_RADIUS = 256.0f;
    constexpr uint8_t PLAYER_TEAM = 0;

    entt::entity CreatePlayer(entt::registry &registry, const TransformComponent &transform,
                              std::shared_ptr<SkinnedMesh> mesh);

    // Places the unit on the terrain surface at position.xz. Returns entt::null if unitItem isn't a unit.
    entt::entity CreateUnit(entt::registry &registry, ItemId unitItem, glm::vec3 position, uint8_t team,
                            std::shared_ptr<SkinnedMesh> mesh, int seed);
}
