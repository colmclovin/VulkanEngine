// PlacementSystem.h
#pragma once
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include "../Game/ItemDatabase.h"

class PlacementSystem {
public:
    void Update(entt::registry& registry, entt::entity player, ItemId selectedItem, float placeDistance);
    void TryConfirmPlacement(entt::registry& registry, entt::entity player);
    void CancelPlacement(entt::registry& registry);

    bool IsPlacing() const { return m_GhostEntity != entt::null; }

private:
    entt::entity m_GhostEntity = entt::null;
    ItemId m_PendingItem = ItemId::None;
};