// InteractionSystem.cpp
#include "InteractionSystem.h"
#include "../Audio/AudioEventSystem.h"
#include "Components.h"
#include <limits>
#include "TerrainRaycast.h"
#include <iostream>
#include "PlacementGrid.h"
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

bool InteractionSystem::TryMineGround(ResourceMap &resourceMap, entt::registry &registry, entt::entity player,
                                      glm::vec3 playerPos, glm::vec3 targetPos, float maxRange,
                                      float extractAmount, AudioEventSystem *audio) {
    float dist = glm::length(targetPos - playerPos);
    if (dist > maxRange) return false; // too far away — out of reach

    ResourceCell *cell = resourceMap.GetCellAtWorldPos(targetPos.x, targetPos.z);
    if (!cell || cell->resource == ItemId::None) return false;

    auto &inventory = registry.get<InventoryComponent>(player);
    inventory.AddItem(cell->resource, 1);
    resourceMap.ExtractFromCell(cell, extractAmount);
    audio->Trigger(AudioEvent::OreCollected);
    return true;
}

bool InteractionSystem::TryMineAtCursor(entt::registry &registry, ResourceMap &resourceMap, entt::entity player,
                                        glm::vec3 rayOrigin, glm::vec3 rayDir, const TerrainSettings &terrainSettings,
                                        float maxRange, AudioEventSystem *audio) {
    auto &playerTransform = registry.get<TransformComponent>(player);

    std::cout << "rayOrigin: " << rayOrigin.x << "," << rayOrigin.y << "," << rayOrigin.z << std::endl;
    std::cout << "rayDir: " << rayDir.x << "," << rayDir.y << "," << rayDir.z << std::endl;

entt::entity target = FindEntityAlongRay(registry, rayOrigin, rayDir, 100.0f); // generous ray cap, not maxRange
    if (registry.valid(target)) {
        float dist = glm::length(registry.get<TransformComponent>(target).Position - playerTransform.Position);
        if (dist <= maxRange) { // THIS is the real gameplay range check
            Mine(registry, target, player, audio);
            return true;
        
        }
    } else {
        std::cout << "No entity found along ray" << std::endl;
    }

    glm::vec3 groundHit = TerrainRaycast::RaycastToTerrain(rayOrigin, rayDir, terrainSettings);
    float groundDist = glm::length(groundHit - playerTransform.Position);
    std::cout << "groundHit: " << groundHit.x << "," << groundHit.y << "," << groundHit.z
              << "  groundDist: " << groundDist << " (maxRange " << maxRange << ")" << std::endl;

    if (groundDist <= maxRange) {
        ResourceCell *cell = resourceMap.GetCellAtWorldPos(groundHit.x, groundHit.z);
        if (cell) {
            std::cout << "Cell found, resource: " << static_cast<int>(cell->resource) << " amount: " << cell->amount << std::endl;
        } else {
            std::cout << "No cell at that position (out of grid bounds?)" << std::endl;
        }
        if (cell && cell->resource != ItemId::None) {
            auto &inventory = registry.get<InventoryComponent>(player);
            inventory.AddItem(cell->resource, 1);
            resourceMap.ExtractFromCell(cell, 10.0f);
            audio->Trigger(AudioEvent::OreCollected);
            return true;
        }
    } else {
        std::cout << "Ground hit out of range" << std::endl;
    }

    return false;
}

static bool RayIntersectsAABB(glm::vec3 rayOrigin, glm::vec3 rayDir, glm::vec3 boxMin, glm::vec3 boxMax, float &outT) {
    float tMin = 0.0f, tMax = std::numeric_limits<float>::max();

    for (int axis = 0; axis < 3; axis++) {
        float invD = 1.0f / rayDir[axis];
        float t0 = (boxMin[axis] - rayOrigin[axis]) * invD;
        float t1 = (boxMax[axis] - rayOrigin[axis]) * invD;
        if (invD < 0.0f) std::swap(t0, t1);
        tMin = std::max(tMin, t0);
        tMax = std::min(tMax, t1);
        if (tMax <= tMin) return false;
    }
    outT = tMin;
    return true;
}

entt::entity InteractionSystem::FindEntityAlongRay(entt::registry &registry, glm::vec3 rayOrigin, glm::vec3 rayDir, float maxDistance) {
    entt::entity closest = entt::null;
    float closestT = maxDistance;

    auto view = registry.view<TransformComponent, BoundsComponent, HarvestableComponent>();
    for (auto entity : view) {
        auto &transform = view.get<TransformComponent>(entity);
        auto &bounds = view.get<BoundsComponent>(entity);

        glm::vec3 center = transform.Position + glm::vec3(0.0f, bounds.halfExtents.y, 0.0f); // assume pivot at base
        glm::vec3 boxMin = center - bounds.halfExtents;
        glm::vec3 boxMax = center + bounds.halfExtents;

        float t;
        if (RayIntersectsAABB(rayOrigin, rayDir, boxMin, boxMax, t) && t < closestT) {
            closestT = t;
            closest = entity;
        }
    }
    return closest;
}

bool InteractionSystem::TryFuelMiner(entt::registry &registry, entt::entity minerEntity, entt::entity player, ItemId selectedItem, int amount) {
    if (!registry.valid(minerEntity) || !registry.any_of<MinerComponent>(minerEntity)) return false;

    auto &miner = registry.get<MinerComponent>(minerEntity);
    if (miner.fuelItem != selectedItem) return false; // must match what's selected, same as furnace/assembler insertion

    auto &inventory = registry.get<InventoryComponent>(player);
    for (auto &slot : inventory.slots) {
        if (slot.item == selectedItem && slot.count > 0) {
            int take = std::min(slot.count, amount);
            slot.count -= take;
            if (slot.count == 0) slot.item = ItemId::None;
            miner.fuelBuffer += take;
            return true;
        }
    }
    return false;
}

bool InteractionSystem::TryCollectMinerOutput(entt::registry &registry, entt::entity minerEntity, entt::entity player) {
    if (!registry.valid(minerEntity) || !registry.any_of<MinerComponent>(minerEntity)) return false;

    auto &miner = registry.get<MinerComponent>(minerEntity);
    if (miner.outputBuffer <= 0) return false;

    auto &inventory = registry.get<InventoryComponent>(player);
    int leftover = inventory.AddItem(miner.outputItem, miner.outputBuffer);
    miner.outputBuffer = leftover; // whatever didn't fit stays in the buffer
    return true;
}

entt::entity InteractionSystem::FindMinerAlongRay(entt::registry &registry, glm::vec3 rayOrigin, glm::vec3 rayDir, float maxDistance) {
    entt::entity closest = entt::null;
    float closestT = maxDistance;

    auto view = registry.view<TransformComponent, BoundsComponent, MinerComponent>();
    for (auto entity : view) {
        auto &transform = view.get<TransformComponent>(entity);
        auto &bounds = view.get<BoundsComponent>(entity);

        glm::vec3 center = transform.Position + glm::vec3(0.0f, bounds.halfExtents.y, 0.0f);
        glm::vec3 boxMin = center - bounds.halfExtents;
        glm::vec3 boxMax = center + bounds.halfExtents;

        float t;
        if (RayIntersectsAABB(rayOrigin, rayDir, boxMin, boxMax, t) && t < closestT) {
            closestT = t;
            closest = entity;
        }
    }
    return closest;
}

entt::entity InteractionSystem::FindMachineAlongRay(entt::registry &registry, glm::vec3 rayOrigin, glm::vec3 rayDir, float maxDistance) {
    entt::entity closest = entt::null;
    float closestT = maxDistance;

    // Matches anything with bounds + an inventory OR a miner — covers furnace, assembler, miner alike
    auto view = registry.view<TransformComponent, BoundsComponent>();
    for (auto entity : view) {
        bool isMachine = registry.any_of<MachineInventoryComponent, MinerComponent, BeltComponent, InserterComponent>(entity);
        (entity);
        if (!isMachine) continue;

        auto &transform = view.get<TransformComponent>(entity);
        auto &bounds = view.get<BoundsComponent>(entity);

        glm::vec3 center = transform.Position + glm::vec3(0.0f, bounds.halfExtents.y, 0.0f);
        glm::vec3 boxMin = center - bounds.halfExtents;
        glm::vec3 boxMax = center + bounds.halfExtents;

        float t;
        if (RayIntersectsAABB(rayOrigin, rayDir, boxMin, boxMax, t) && t < closestT) {
            closestT = t;
            closest = entity;
        }
    }
    return closest;
}   
bool InteractionSystem::TryInsertIntoMachine(entt::registry &registry, entt::entity machine, entt::entity player, ItemId item, int amount) {
    if (!registry.valid(machine) || !registry.any_of<MachineInventoryComponent>(machine)) return false;

    auto &machineInv = registry.get<MachineInventoryComponent>(machine);
    auto &inventory = registry.get<InventoryComponent>(player);

    for (auto &slot : inventory.slots) {
        if (slot.item == item && slot.count > 0) {
            int take = std::min(slot.count, amount);
            int leftover = MachineInventoryComponent::AddToSlots(machineInv.inputs, item, take);
            int actuallyTaken = take - leftover;
            if (actuallyTaken <= 0) return false; // machine's input was already full

            slot.count -= actuallyTaken;
            if (slot.count == 0) slot.item = ItemId::None;
            return true;
        }
    }
    return false;
}

bool InteractionSystem::TryCollectFromMachine(entt::registry &registry, entt::entity machine, entt::entity player) {
    if (!registry.valid(machine) || !registry.any_of<MachineInventoryComponent>(machine)) return false;

    auto &machineInv = registry.get<MachineInventoryComponent>(machine);
    auto &inventory = registry.get<InventoryComponent>(player);

    bool collectedAny = false;
    for (auto &outSlot : machineInv.outputs) {
        if (outSlot.item == ItemId::None || outSlot.count <= 0) continue;

        int leftover = inventory.AddItem(outSlot.item, outSlot.count);
        int actuallyCollected = outSlot.count - leftover;
        if (actuallyCollected > 0) {
            outSlot.count = leftover;
            if (outSlot.count == 0) outSlot.item = ItemId::None;
            collectedAny = true;
        }
    }
    return collectedAny;
}
bool InteractionSystem::TryFuelFurnace(entt::registry &registry, entt::entity furnaceEntity, entt::entity player, ItemId selectedItem, int amount) {
    if (!registry.valid(furnaceEntity) || !registry.any_of<FurnaceComponent>(furnaceEntity)) return false;

    auto &furnace = registry.get<FurnaceComponent>(furnaceEntity);
    if (furnace.fuelItem != selectedItem) return false;

    auto &inventory = registry.get<InventoryComponent>(player);
    for (auto &slot : inventory.slots) {
        if (slot.item == selectedItem && slot.count > 0) {
            int take = std::min(slot.count, amount);
            slot.count -= take;
            if (slot.count == 0) slot.item = ItemId::None;
            furnace.fuelBuffer += take;
            return true;
        }
    }
    return false;
}
bool InteractionSystem::TryRotateMachine(entt::registry &registry, entt::entity target) {
    if (!registry.valid(target)) return false;

    glm::vec3 *facingPtr = nullptr;
    if (registry.any_of<BeltComponent>(target)) {
        facingPtr = &registry.get<BeltComponent>(target).direction;
    } else if (registry.any_of<InserterComponent>(target)) {
        facingPtr = &registry.get<InserterComponent>(target).facing;
    } else {
        return false; // only belts/inserters have a meaningful facing to rotate
    }

    // Rotate 90 degrees: (x,z) -> (-z,x), same rotation step your placement ghost uses
    glm::vec3 old = *facingPtr;
    *facingPtr = glm::vec3(-old.z, 0.0f, old.x);

    float angle = atan2(facingPtr->x, facingPtr->z);
    registry.get<TransformComponent>(target).Rotation = glm::angleAxis(angle, glm::vec3(0, 1, 0));

    return true;
}
bool InteractionSystem::TryPickupMachine(entt::registry &registry, entt::entity target, entt::entity player, PlacementGrid &placementGrid, float gridSize) {
    if (!registry.valid(target)) return false;

    ItemId machineItem = ItemId::None;
    glm::vec3 halfExtents = glm::vec3(0.5f);

    if (registry.any_of<BoundsComponent>(target)) {
        halfExtents = registry.get<BoundsComponent>(target).halfExtents;
    }

    // Return any held items to the player first
    auto &inventory = registry.get<InventoryComponent>(player);

    if (registry.any_of<MinerComponent>(target)) {
        auto &miner = registry.get<MinerComponent>(target);
        if (miner.outputItem != ItemId::None && miner.outputBuffer > 0) {
            inventory.AddItem(miner.outputItem, miner.outputBuffer);
        }
        machineItem = ItemId::Miner;
    } else if (registry.any_of<FurnaceComponent>(target)) {
        machineItem = ItemId::Furnace;
    } else if (registry.any_of<AssemblerComponent>(target)) {
        machineItem = ItemId::Assembler;
    } else if (registry.any_of<BeltComponent>(target)) {
        auto &belt = registry.get<BeltComponent>(target);
        for (auto &item : belt.leftLane.queue)
            if (item.item != ItemId::None) inventory.AddItem(item.item, 1);
        for (auto &item : belt.rightLane.queue)
            if (item.item != ItemId::None) inventory.AddItem(item.item, 1);
        machineItem = ItemId::Belt;
    } else if (registry.any_of<InserterComponent>(target)) {
        auto &inserter = registry.get<InserterComponent>(target);
        if (inserter.holdingItem && inserter.heldItem != ItemId::None) {
            inventory.AddItem(inserter.heldItem, 1);
        }
        machineItem = ItemId::Inserter;
    } else {
        return false; // not a recognized machine type
    }

    // MachineInventoryComponent covers furnace/assembler input+output slots
    if (registry.any_of<MachineInventoryComponent>(target)) {
        auto &inv = registry.get<MachineInventoryComponent>(target);
        for (auto &slot : inv.inputs)
            if (slot.item != ItemId::None) inventory.AddItem(slot.item, slot.count);
        for (auto &slot : inv.outputs)
            if (slot.item != ItemId::None) inventory.AddItem(slot.item, slot.count);
    }

    // Free the grid tiles this machine occupied
    auto &transform = registry.get<TransformComponent>(target);
    auto coveredCells = PlacementGrid::GetCoveredCells(transform.Position, halfExtents, gridSize);
    placementGrid.UnregisterArea(coveredCells);

    // Give the machine item itself back
    inventory.AddItem(machineItem, 1);

    registry.destroy(target);
    return true;
}