// InteractionSystem.h
#pragma once
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include "../Game/ItemDatabase.h"
#include "../Game/WorldState.h"
#include "../Utils/GameSettings.h"

class AudioEventSystem;
class VulkanEngine;
class MeshRenderer;
class ChunkManager;
class TreeRenderer;

// Player <-> world interactions: picking things under the cursor, mining, and moving items
// between the player's inventory and machines.
class InteractionSystem {
public:
    // --- Picking (ray vs. entity bounding boxes) ---
    static entt::entity FindMachineAlongRay(entt::registry &registry, glm::vec3 rayOrigin, glm::vec3 rayDir, float maxDistance);
    static entt::entity FindSelectableAlongRay(entt::registry &registry, glm::vec3 rayOrigin, glm::vec3 rayDir, float maxDistance);
    static entt::entity FindDamageableAlongRay(entt::registry &registry, glm::vec3 rayOrigin, glm::vec3 rayDir, float maxDistance);
    static entt::entity FindEnemyAlongRay(entt::registry &registry, glm::vec3 rayOrigin, glm::vec3 rayDir, float maxDistance, uint8_t myTeam);

    // --- Pickups ---
    static entt::entity FindNearestPickup(entt::registry &registry, glm::vec3 playerPos, float range);
    static void CollectPickup(entt::registry &registry, entt::entity pickup, entt::entity player, AudioEventSystem *audio);

    // Mines ore at the cursor's ground point, or else chops the tree under the cursor. Returns true if anything happened.
    static bool TryMineAtCursor(entt::registry &registry, WorldState &world, entt::entity player, glm::vec3 rayOrigin,
                                glm::vec3 rayDir, const TerrainSettings &terrainSettings, float maxRange,
                                VulkanEngine *engine, MeshRenderer *meshRenderer, ChunkManager &chunkManager,
                                const TreeRenderer &treeRenderer);

    // --- Machines ---
    // Loads fuel into a furnace, generator or miner. Fails if the item isn't fuel or a different fuel is loaded.
    static bool TryFuelMachine(entt::registry &registry, entt::entity machine, entt::entity player, ItemId item, int amount);
    static bool TryInsertIntoMachine(entt::registry &registry, entt::entity machine, entt::entity player, ItemId item, int amount);
    static bool TryCollectFromMachine(entt::registry &registry, entt::entity machine, entt::entity player);
    static bool TryCollectMinerOutput(entt::registry &registry, entt::entity minerEntity, entt::entity player);
    static bool TryRotateMachine(entt::registry &registry, entt::entity target);
    static bool TryPickupMachine(entt::registry &registry, entt::entity target, entt::entity player,
                                 PlacementGrid &placementGrid, float gridSize);
};
