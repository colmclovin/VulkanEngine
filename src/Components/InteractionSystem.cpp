// InteractionSystem.cpp
#include "InteractionSystem.h"
#include "../Audio/AudioEventSystem.h"
#include "Components.h"
#include <limits>

entt::entity InteractionSystem::FindNearestInteractable(entt::registry &registry, glm::vec3 playerPos, float range) {
    entt::entity closest = entt::null;
    float closestDist = std::numeric_limits<float>::max();

    auto view = registry.view<TransformComponent, HarvestableComponent>();
    for (auto entity : view) {
        auto &transform = view.get<TransformComponent>(entity);
        float dist = glm::length(transform.Position - playerPos);
        if (dist <= range && dist < closestDist) {
            closestDist = dist;
            closest = entity;
        }
    }
    return closest;
}

entt::entity InteractionSystem::FindNearestPickup(entt::registry &registry, glm::vec3 playerPos, float range) {
    entt::entity closest = entt::null;
    float closestDist = std::numeric_limits<float>::max();

    auto view = registry.view<TransformComponent, PickupComponent>();
    for (auto entity : view) {
        auto &transform = view.get<TransformComponent>(entity);
        float dist = glm::length(transform.Position - playerPos);
        if (dist <= range && dist < closestDist) {
            closestDist = dist;
            closest = entity;
        }
    }
    return closest;
}

void InteractionSystem::Mine(entt::registry &registry, entt::entity target, entt::entity player, AudioEventSystem *audio) {
    if (!registry.valid(target) || !registry.any_of<HarvestableComponent>(target)) return;

    auto &harvest = registry.get<HarvestableComponent>(target);
    auto &inventory = registry.get<InventoryComponent>(player);

    harvest.health -= 25.0f; // tune per-hit damage as needed

    //inventory.AddItem(harvest.yieldItem, harvest.yieldPerHit);
    audio->Trigger(AudioEvent::TreeChopped);

    
        auto &targetTransform = registry.get<TransformComponent>(target);

        float scatterRadius = 1.0f;   // tune to taste
        float angle = static_cast<float>(rand()) / RAND_MAX * glm::two_pi<float>();
        float distance = static_cast<float>(rand()) / RAND_MAX * scatterRadius;
        glm::vec3 offset(cos(angle) * distance, 0.0f, sin(angle) * distance);



        auto pickupEntity = registry.create();
        auto &pickupTransform = registry.emplace<TransformComponent>(pickupEntity);
        pickupTransform.Position = targetTransform.Position + offset;
		pickupTransform.Scale = glm::vec3(0.3f); 
        registry.emplace<PickupComponent>(pickupEntity, PickupComponent{ harvest.yieldItem, harvest.yieldOnDestroy });
        // TODO: give it a small mesh (a dropped-item model) via MeshComponent once you have one
        auto dropMesh = ItemDatabase::GetWorldMesh(harvest.yieldItem);   // CHANGED — looked up, not stored
        if (dropMesh) {
            registry.emplace<MeshComponent>(pickupEntity, dropMesh);
        }
    if (harvest.health <= 0.0f) {
        registry.destroy(target);
    }
}

void InteractionSystem::CollectPickup(entt::registry &registry, entt::entity pickup, entt::entity player, AudioEventSystem *audio) {
    if (!registry.valid(pickup) || !registry.any_of<PickupComponent>(pickup)) return;

    auto &pickupComp = registry.get<PickupComponent>(pickup);
    auto &inventory = registry.get<InventoryComponent>(player);

    int leftover = inventory.AddItem(pickupComp.item, pickupComp.count);
    if (leftover == 0) {
        // Fully picked up
        registry.destroy(pickup);
        audio->Trigger(AudioEvent::OreCollected); // rename to something generic like ItemPickup later
    } else {
        pickupComp.count = leftover; // partial pickup if inventory was nearly full
    }
}