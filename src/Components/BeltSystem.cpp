// BeltSystem.cpp
#include "BeltSystem.h"
#include "BeltComponent.h"
#include "Components.h"
#include "MachineInventoryComponent.h"

void BeltSystem::Update(entt::registry &registry, PlacementGrid &grid, float gridSize, float deltaTime) {
    auto view = registry.view<TransformComponent, BeltComponent>();
    for (auto entity : view) {
        auto &transform = view.get<TransformComponent>(entity);
        auto &belt = view.get<BeltComponent>(entity);

        if (belt.heldItem == ItemId::None) continue;

        belt.progress += belt.speed * deltaTime;
        if (belt.progress < 1.0f) continue;

        // Ready to hand off — find what's on the next tile in our facing direction
        GridCoord myCoord = PlacementGrid::WorldToGrid(transform.Position, gridSize);
        GridCoord nextCoord = { myCoord.x + static_cast<int>(std::round(belt.direction.x)),
                                myCoord.z + static_cast<int>(std::round(belt.direction.z)) };

        entt::entity nextEntity = grid.GetEntityAt(nextCoord);
        if (!registry.valid(nextEntity)) continue; // nothing ahead — item waits at progress=1

        if (registry.any_of<BeltComponent>(nextEntity)) {
            auto &nextBelt = registry.get<BeltComponent>(nextEntity);
            if (nextBelt.heldItem == ItemId::None) {
                nextBelt.heldItem = belt.heldItem;
                nextBelt.progress = 0.0f;
                belt.heldItem = ItemId::None;
                belt.progress = 0.0f;
            }
        } else if (registry.any_of<MachineInventoryComponent>(nextEntity)) {
            auto &nextInv = registry.get<MachineInventoryComponent>(nextEntity);
            int leftover = MachineInventoryComponent::AddToSlots(nextInv.inputs, belt.heldItem, 1);
            if (leftover == 0) {
                belt.heldItem = ItemId::None;
                belt.progress = 0.0f;
            }
        }
    }
}