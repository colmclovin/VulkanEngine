// InteractionSystem.h
#pragma once
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include "ResourceMap.h"
#include "GameSettings.h"
class AudioEventSystem;

class InteractionSystem {
public:
    static entt::entity FindNearestInteractable(entt::registry &registry, glm::vec3 playerPos, float range);
    static entt::entity FindNearestPickup(entt::registry &registry, glm::vec3 playerPos, float range);
    static void CollectPickup(entt::registry &registry, entt::entity pickup, entt::entity player, AudioEventSystem *audio);
    static void Mine(entt::registry &registry, entt::entity target, entt::entity player, AudioEventSystem *audio);
    static bool TryMineGround(ResourceMap &resourceMap, entt::registry &registry, entt::entity player,
                              glm::vec3 playerPos, glm::vec3 targetPos, float maxRange,
                              float extractAmount, AudioEventSystem *audio);
    static bool TryMineAtCursor(entt::registry &registry, ResourceMap &resourceMap, entt::entity player,
                                glm::vec3 rayOrigin, glm::vec3 rayDir, const TerrainSettings &terrainSettings,
                                float maxRange, AudioEventSystem *audio);
    static entt::entity FindEntityAlongRay(entt::registry &registry, glm::vec3 rayOrigin, glm::vec3 rayDir, float maxDistance);
    static bool TryFuelMiner(entt::registry &registry, entt::entity minerEntity, entt::entity player, ItemId fuelItem, int amount);
    static bool TryCollectMinerOutput(entt::registry &registry, entt::entity minerEntity, entt::entity player);
    static entt::entity FindMinerAlongRay(entt::registry &registry, glm::vec3 rayOrigin, glm::vec3 rayDir, float maxDistance);
};