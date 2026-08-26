// PlacementSystem.cpp
#include "PlacementSystem.h"
#include "Components.h"
#include "../Game/PlaceableDatabase.h"
#include "../Components/ModelLoader.h"

void PlacementSystem::Update(entt::registry& registry, entt::entity player, ItemId selectedItem, float placeDistance) {
    const PlaceableDef* def = PlaceableDatabase::TryGet(selectedItem);

    if (!def) {
        // Selected item isn't placeable — clear any existing ghost
        CancelPlacement(registry);
        return;
    }

    if (!registry.valid(player)) return;
    auto& playerTransform = registry.get<TransformComponent>(player);

    // Position the ghost in front of the player, along their facing direction.
    // Reuse the same forward-vector logic as MovePlayer for consistency.
    // (Pass camera yaw in if you want camera-relative facing instead of last-move direction.)
    glm::vec3 ghostPos = playerTransform.Position + glm::vec3(0.0f, 0.0f, placeDistance); // placeholder direction

    if (m_GhostEntity == entt::null || m_PendingItem != selectedItem) {
        CancelPlacement(registry);   // remove stale ghost if item selection changed

        m_GhostEntity = registry.create();
        registry.emplace<TransformComponent>(m_GhostEntity).Position = ghostPos;
        auto mesh = std::make_shared<Mesh>(ModelLoader::LoadModel(def->meshPath));
        registry.emplace<MeshComponent>(m_GhostEntity, mesh);
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