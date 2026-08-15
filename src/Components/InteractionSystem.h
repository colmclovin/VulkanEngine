// InteractionSystem.h
#pragma once
#include <entt/entt.hpp>
#include <glm/glm.hpp>
class AudioEventSystem;

class InteractionSystem {
public:
    static entt::entity FindNearestInteractable(entt::registry &registry, glm::vec3 playerPos, float range);
    static entt::entity FindNearestPickup(entt::registry &registry, glm::vec3 playerPos, float range);
    static void CollectPickup(entt::registry &registry, entt::entity pickup, entt::entity player, AudioEventSystem *audio);
    static void Mine(entt::registry &registry, entt::entity target, entt::entity player, AudioEventSystem *audio);
};