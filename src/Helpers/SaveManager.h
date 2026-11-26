#pragma once
#include "../World/PlacementGrid.h"
#include "../World/ResourceMap.h"
#include "../Game/TechState.h"
#include <entt/entt.hpp>
#include <string>
#include "../Engine/VulkanEngine.h"
#include "../Renderer/MeshRenderer.h"
#include "../World/RemovedTreesMap.h"
#include "../World/DepletionMap.h"
struct GameSettings;
class SkinnedMesh;

struct SavedUnit {
    ItemId item;
    glm::vec3 pos;
    float health;
    uint8_t team;
};

class SaveManager {
public:
    static void SaveGame(const std::string &path, entt::registry &registry, entt::entity player,
                         ResourceMap &resourceMap, TechState &techState, const GameSettings &settings, VulkanEngine *engine, MeshRenderer *meshRenderer, DepletionMap &depletionMap, RemovedTreesMap &removedTreesMap);
    static bool LoadGame(const std::string &path, entt::registry &registry, entt::entity &outPlayer,
                         ResourceMap &resourceMap, PlacementGrid &placementGrid, TechState &techState,
                         GameSettings &settings, VulkanEngine *engine, MeshRenderer *meshRenderer,
                         DepletionMap &depletionMap, RemovedTreesMap &removedTreesMap,
                         std::shared_ptr<SkinnedMesh> unitMesh = nullptr, // optional
                         std::vector<SavedUnit> *outUnits = nullptr);

private:
    std::vector<SavedUnit> *outUnits = nullptr;
};