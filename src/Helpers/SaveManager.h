#pragma once
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>
#include "../Game/ItemDatabase.h"
#include "../Game/WorldState.h"

struct GameSettings;
class SkinnedMesh;
class VulkanEngine;
class MeshRenderer;

struct SavedUnit {
    ItemId item;
    glm::vec3 pos;
    float health;
    uint8_t team;
};

class SaveManager {
public:
    static void SaveGame(const std::string &path, entt::registry &registry, entt::entity player, const WorldState &world,
                         const GameSettings &settings);

    // Expects an empty registry and a fresh WorldState. Units aren't created here; they're returned in outUnits
    // so the caller can spawn them.
    static bool LoadGame(const std::string &path, entt::registry &registry, entt::entity &outPlayer, WorldState &world,
                         GameSettings &settings, VulkanEngine *engine, MeshRenderer *meshRenderer,
                         std::shared_ptr<SkinnedMesh> unitMesh, std::vector<SavedUnit> &outUnits);
};
