// BeltSystem.cpp
#include "BeltSystem.h"
#include "BeltComponent.h"
#include "Components.h"
#include "MachineInventoryComponent.h"
#include "PlacementGrid.h"
#include <iostream>

static void UpdateLane(entt::registry &registry, BeltLane &lane, float speed, float deltaTime,
                       entt::entity nextEntity, bool isLeftLane) {
    if (lane.queue.empty()) return;

    for (auto &item : lane.queue) {
        item.progress = std::min(item.progress + speed * deltaTime, 1.0f);
    }

    auto &front = lane.queue.front();
    if (front.progress < 1.0f) return;
    if (!registry.valid(nextEntity)) return;

    bool delivered = false;
    if (registry.any_of<BeltComponent>(nextEntity)) {
        auto &nextBelt = registry.get<BeltComponent>(nextEntity);
        BeltLane &targetLane = isLeftLane ? nextBelt.leftLane : nextBelt.rightLane;
        if ((int)targetLane.queue.size() < targetLane.capacity) {
            targetLane.queue.push_back({ front.item, 0.0f });
            delivered = true;
        }
    } else if (registry.any_of<MachineInventoryComponent>(nextEntity)) {
        auto &nextInv = registry.get<MachineInventoryComponent>(nextEntity);
        int leftover = MachineInventoryComponent::AddToSlots(nextInv.inputs, front.item, 1);
        delivered = (leftover == 0);
    }

    if (delivered) {
        lane.queue.erase(lane.queue.begin());
    }
}

void BeltSystem::Update(entt::registry &registry, PlacementGrid &grid, float gridSize, float deltaTime) {
    auto view = registry.view<TransformComponent, BeltComponent, BoundsComponent>(); // ADD BoundsComponent to the view
    for (auto entity : view) {
        auto &transform = view.get<TransformComponent>(entity);
        auto &belt = view.get<BeltComponent>(entity);
        auto &bounds = view.get<BoundsComponent>(entity); // ADD

        entt::entity nextEntity = FindNeighborInDirection(registry, grid, transform.Position, bounds.halfExtents, belt.direction, gridSize);


        std::cout << "Belt at (" << transform.Position.x << "," << transform.Position.z
                  << ") facing (" << belt.direction.x << "," << belt.direction.z
                  << ") found next entity: " << (registry.valid(nextEntity) ? "yes" : "NONE") << std::endl;

        UpdateLane(registry, belt.leftLane, belt.speed, deltaTime, nextEntity, true);
        UpdateLane(registry, belt.rightLane, belt.speed, deltaTime, nextEntity, false);
    }
}