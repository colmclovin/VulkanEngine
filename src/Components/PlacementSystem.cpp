// PlacementSystem.cpp
#include "PlacementSystem.h"
#include "Components.h"
#include "../Game/PlaceableDatabase.h"
#include "../Components/ModelLoader.h"
#include "TerrainGenerator.h"

void PlacementSystem::Update(entt::registry& registry, entt::entity player, Camera3D& camera,
    ItemId selectedItem, const TerrainSettings& terrainSettings,
    float mouseX, float mouseY, float screenWidth, float screenHeight, float aspect) {
    const PlaceableDef* def = PlaceableDatabase::TryGet(selectedItem);
    if (!def) {
        CancelPlacement(registry);
        return;
    }

    glm::vec3 rayOrigin = camera.GetEyePosition();
    glm::vec3 rayDir = camera.ScreenPointToRay(mouseX, mouseY, screenWidth, screenHeight, aspect);
    glm::vec3 hitPoint = RaycastToTerrain(rayOrigin, rayDir, terrainSettings);

    float gridSize = 1.0f;   // tune — match your terrain's cellSize for visually clean alignment
    glm::vec3 ghostPos = SnapToGrid(hitPoint, gridSize);

    if (m_GhostEntity == entt::null || m_PendingItem != selectedItem) {
        CancelPlacement(registry);

        m_GhostEntity = registry.create();
        registry.emplace<TransformComponent>(m_GhostEntity).Position = ghostPos;
        auto mesh = ItemDatabase::GetWorldMesh(selectedItem);   // reuse the centralized lookup from earlier
        if (mesh) registry.emplace<MeshComponent>(m_GhostEntity, mesh);
        registry.emplace<GhostComponent>(m_GhostEntity);
        m_PendingItem = selectedItem;
    }
    else {
        registry.get<TransformComponent>(m_GhostEntity).Position = ghostPos;
    }
}

void PlacementSystem::TryConfirmPlacement(entt::registry& registry, entt::entity player) {
    if (m_GhostEntity == entt::null) return;

    auto& inventory = registry.get<InventoryComponent>(player);

    // Check the player actually has the item, consume one
    for (auto& slot : inventory.slots) {
        if (slot.item == m_PendingItem && slot.count > 0) {
            slot.count--;
            if (slot.count == 0) slot.item = ItemId::None;

            // Promote the ghost into a real placed entity
            registry.remove<GhostComponent>(m_GhostEntity);
            // TODO: emplace whatever "this is a real placed building" component you want here

            m_GhostEntity = entt::null;
            m_PendingItem = ItemId::None;
            return;
        }
    }
    // No item available — placement fails silently, or you could feed back to the player here
}

void PlacementSystem::CancelPlacement(entt::registry& registry) {
    if (m_GhostEntity != entt::null && registry.valid(m_GhostEntity)) {
        registry.destroy(m_GhostEntity);
    }
    m_GhostEntity = entt::null;
    m_PendingItem = ItemId::None;
}

glm::vec3 PlacementSystem::RaycastToTerrain(glm::vec3 rayOrigin, glm::vec3 rayDir, const TerrainSettings& terrainSettings) {
    // March along the ray in small steps, checking when it crosses the terrain height at that XZ.
    // Simple and robust for a heightmap; avoids needing real mesh-triangle intersection.
    const float stepSize = 0.25f;
    const float maxDistance = 200.0f;

    glm::vec3 pos = rayOrigin;
    for (float t = 0.0f; t < maxDistance; t += stepSize) {
        pos = rayOrigin + rayDir * t;
        float terrainY = TerrainGenerator::SampleHeight(pos.x, pos.z, terrainSettings);
        if (pos.y <= terrainY) {
            return glm::vec3(pos.x, terrainY, pos.z);   // hit — snap Y exactly to terrain surface
        }
    }
    return rayOrigin + rayDir * maxDistance;   // no hit within range — fallback point far along the ray
}
glm::vec3 PlacementSystem::SnapToGrid(glm::vec3 pos, float gridSize) {   // ADD PlacementSystem:: qualifier
    return glm::vec3(
        std::round(pos.x / gridSize) * gridSize,
        pos.y,
        std::round(pos.z / gridSize) * gridSize
    );
}