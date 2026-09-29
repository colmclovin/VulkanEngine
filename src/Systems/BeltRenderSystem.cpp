// BeltRenderSystem.cpp
#include "BeltRenderSystem.h"
#include "../Components/BeltComponent.h"
#include "../Components/Components.h"
#include "../Game/ItemDatabase.h"

 void BeltRenderSystem::SyncLane(entt::registry& registry, BeltLane& lane, glm::vec3 beltPosition, glm::vec3 direction,
    float lateralOffset, VulkanEngine* engine, MeshRenderer* meshRenderer, float gridSize) {
    // Ensure we have exactly one rendering entity per queued item
    while (lane.itemEntities.size() < lane.queue.size()) {
        auto entity = registry.create();
        registry.emplace<TransformComponent>(entity);
        lane.itemEntities.push_back(entity);
    }
    while (lane.itemEntities.size() > lane.queue.size()) {
        auto entity = lane.itemEntities.back();
        if (registry.valid(entity)) registry.destroy(entity);
        lane.itemEntities.pop_back();
    }

    glm::vec3 right = glm::normalize(glm::cross(direction, glm::vec3(0, 1, 0)));

    for (size_t i = 0; i < lane.queue.size(); i++) {
        auto& item = lane.queue[i];
        auto entity = lane.itemEntities[i];
        if (!registry.valid(entity)) continue;

        // Position along the belt: progress 0 = entry edge, progress 1 = exit edge
        glm::vec3 entryPos = beltPosition - direction * (gridSize * 0.5f) + right * lateralOffset;
        glm::vec3 exitPos = beltPosition + direction * (gridSize * 0.5f) + right * lateralOffset;
        glm::vec3 worldPos = glm::mix(entryPos, exitPos, item.progress);
        worldPos.y += 0.3f;   // sit slightly above the belt surface

        auto& transform = registry.get<TransformComponent>(entity);
        transform.Position = worldPos;
        transform.Scale = glm::vec3(0.3f);   // small, matching pickup item scale

        // Ensure the correct mesh is attached (item type could change between syncs if queue contents shifted)
        if (!registry.any_of<MeshComponent>(entity) ||
            (registry.any_of<MeshComponent>(entity) /* and mesh doesn't match item.item — see note below */)) {
            auto mesh = ItemDatabase::GetWorldMesh(item.item, engine, meshRenderer);
            if (mesh) registry.emplace_or_replace<MeshComponent>(entity, mesh);
        }
    }
}

void BeltRenderSystem::SyncItemEntities(entt::registry& registry, VulkanEngine* engine, MeshRenderer* meshRenderer, float gridSize) {
    auto view = registry.view<TransformComponent, BeltComponent>();
    for (auto entity : view) {
        auto& transform = view.get<TransformComponent>(entity);
        auto& belt = view.get<BeltComponent>(entity);

        glm::vec3 direction = glm::normalize(belt.direction);
        float laneOffset = gridSize * 0.2f;   // small lateral offset so left/right lanes don't overlap visually

        SyncLane(registry, belt.leftLane, transform.Position, direction, -laneOffset, engine, meshRenderer, gridSize);
        SyncLane(registry, belt.rightLane, transform.Position, direction, laneOffset, engine, meshRenderer, gridSize);
    }
}