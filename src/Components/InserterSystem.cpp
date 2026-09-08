// InserterSystem.cpp
#include "InserterSystem.h"
#include "BeltComponent.h"
#include "Components.h"
#include "InserterComponent.h"
#include "MachineInventoryComponent.h"
#include "PlacementGrid.h"

void InserterSystem::Update(entt::registry &registry, PlacementGrid &grid, float gridSize, float deltaTime) {
    auto view = registry.view<TransformComponent, InserterComponent, BoundsComponent>();
    for (auto entity : view) {
        auto &transform = view.get<TransformComponent>(entity);
        auto &inserter = view.get<InserterComponent>(entity);
        auto &bounds = view.get<BoundsComponent>(entity);

        glm::vec3 behindDir = -inserter.facing;
        glm::vec3 aheadDir = inserter.facing;

        if (!inserter.holdingItem) {
            entt::entity sourceEntity = FindNeighborInDirection(registry, grid, transform.Position, bounds.halfExtents, behindDir, gridSize);
            if (!registry.valid(sourceEntity)) continue;

            if (registry.any_of<MinerComponent>(sourceEntity)) {
                auto &miner = registry.get<MinerComponent>(sourceEntity);
                if (miner.outputItem != ItemId::None && miner.outputBuffer > 0) {
                    miner.outputBuffer--;
                    inserter.heldItem = miner.outputItem;
                    inserter.holdingItem = true;
                    inserter.timer = inserter.swingTime;
                }
            } 
            else if (registry.any_of<MachineInventoryComponent>(sourceEntity)) {
                auto &sourceInv = registry.get<MachineInventoryComponent>(sourceEntity);
                for (auto &slot : sourceInv.outputs) {
                    if (slot.item != ItemId::None && slot.count > 0) {
                        ItemId pickedUpItem = slot.item; // capture BEFORE mutating the slot
                        slot.count--;
                        if (slot.count == 0) slot.item = ItemId::None;

                        inserter.heldItem = pickedUpItem; // use the captured value, not the (possibly cleared) slot
                        inserter.holdingItem = true;
                        inserter.timer = inserter.swingTime;
                        break;
                    }
                }
            } else if (registry.any_of<BeltComponent>(sourceEntity)) {
                auto &sourceBelt = registry.get<BeltComponent>(sourceEntity);

                BeltLane *lane = nullptr;
                if (!sourceBelt.leftLane.queue.empty() && sourceBelt.leftLane.queue.front().progress >= 1.0f) {
                    lane = &sourceBelt.leftLane;
                } else if (!sourceBelt.rightLane.queue.empty() && sourceBelt.rightLane.queue.front().progress >= 1.0f) {
                    lane = &sourceBelt.rightLane;
                }

                if (lane) {
                    inserter.heldItem = lane->queue.front().item;
                    inserter.holdingItem = true;
                    inserter.timer = inserter.swingTime;
                    lane->queue.erase(lane->queue.begin());
                }
            }
        } else {
            inserter.timer -= deltaTime;
            if (inserter.timer > 0.0f) continue;

            entt::entity targetEntity = FindNeighborInDirection(registry, grid, transform.Position, bounds.halfExtents, aheadDir, gridSize);
            if (!registry.valid(targetEntity)) continue;

            bool delivered = false;
            if (registry.any_of<MachineInventoryComponent>(targetEntity)) {
                auto &targetInv = registry.get<MachineInventoryComponent>(targetEntity);
                int leftover = MachineInventoryComponent::AddToSlots(targetInv.inputs, inserter.heldItem, 1);
                delivered = (leftover == 0);
            } else if (registry.any_of<BeltComponent>(targetEntity)) {
                auto &targetBelt = registry.get<BeltComponent>(targetEntity);

                BeltLane *lane = nullptr;
                if ((int)targetBelt.leftLane.queue.size() < targetBelt.leftLane.capacity) {
                    lane = &targetBelt.leftLane;
                } else if ((int)targetBelt.rightLane.queue.size() < targetBelt.rightLane.capacity) {
                    lane = &targetBelt.rightLane;
                }

                if (lane) {
                    lane->queue.push_back({ inserter.heldItem, 0.0f });
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