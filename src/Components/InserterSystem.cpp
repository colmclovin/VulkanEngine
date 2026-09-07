// InserterSystem.cpp
#include "InserterSystem.h"
#include "Components.h"
#include "InserterComponent.h"
#include "MachineInventoryComponent.h"

void InserterSystem::Update(entt::registry &registry, PlacementGrid &grid, float gridSize, float deltaTime) {
    auto view = registry.view<TransformComponent, InserterComponent>();
    for (auto entity : view) {
        auto &transform = view.get<TransformComponent>(entity);
        auto &inserter = view.get<InserterComponent>(entity);

        GridCoord myCoord = PlacementGrid::WorldToGrid(transform.Position, gridSize);
        GridCoord sourceCoord = { myCoord.x - static_cast<int>(std::round(inserter.facing.x)),
                                  myCoord.z - static_cast<int>(std::round(inserter.facing.z)) };
        GridCoord targetCoord = { myCoord.x + static_cast<int>(std::round(inserter.facing.x)),
                                  myCoord.z + static_cast<int>(std::round(inserter.facing.z)) };

        if (!inserter.holdingItem) {
            entt::entity sourceEntity = grid.GetEntityAt(sourceCoord);
            if (!registry.valid(sourceEntity)) continue;

            if (registry.any_of<MachineInventoryComponent>(sourceEntity)) {
                auto &sourceInv = registry.get<MachineInventoryComponent>(sourceEntity);
                for (auto &slot : sourceInv.outputs) {
                    if (slot.item != ItemId::None && slot.count > 0) {
                        slot.count--;
                        if (slot.count == 0) slot.item = ItemId::None;
                        inserter.heldItem = slot.item;
                        inserter.holdingItem = true;
                        inserter.timer = inserter.swingTime;
                        break;
                    }
                }
            } else if (registry.any_of<BeltComponent>(sourceEntity)) {
                auto &sourceBelt = registry.get<BeltComponent>(sourceEntity);
                if (sourceBelt.heldItem != ItemId::None && sourceBelt.progress >= 1.0f) {
                    inserter.heldItem = sourceBelt.heldItem;
                    inserter.holdingItem = true;
                    inserter.timer = inserter.swingTime;
                    sourceBelt.heldItem = ItemId::None;
                    sourceBelt.progress = 0.0f;
                }
            }
        } else {
            inserter.timer -= deltaTime;
            if (inserter.timer > 0.0f) continue;

            entt::entity targetEntity = grid.GetEntityAt(targetCoord);
            if (!registry.valid(targetEntity)) continue;

            bool delivered = false;
            if (registry.any_of<MachineInventoryComponent>(targetEntity)) {
                auto &targetInv = registry.get<MachineInventoryComponent>(targetEntity);
                int leftover = MachineInventoryComponent::AddToSlots(targetInv.inputs, inserter.heldItem, 1);
                delivered = (leftover == 0);
            } else if (registry.any_of<BeltComponent>(targetEntity)) {
                auto &targetBelt = registry.get<BeltComponent>(targetEntity);
                if (targetBelt.heldItem == ItemId::None) {
                    targetBelt.heldItem = inserter.heldItem;
                    targetBelt.progress = 0.0f;
                    delivered = true;
                }
            }

            if (delivered) {
                inserter.holdingItem = false;
                inserter.heldItem = ItemId::None;
            }
        }
    }
}