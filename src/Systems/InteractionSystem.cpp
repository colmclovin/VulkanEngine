// InteractionSystem.cpp
#include "InteractionSystem.h"
#include "../Audio/AudioEventSystem.h"
#include "../Components/Components.h"
#include "../Game/ChunkManager.h"
#include "../Game/FuelDatabase.h"
#include "../Game/OreDepositMap.h"
#include "../Renderer/TreeRenderer.h"
#include "../Utils/RayMath.h"
#include "../World/TerrainRaycast.h"
#include "../World/TreeRaycast.h"
#include <cstdlib>
#include <limits>

namespace {
    constexpr float TREE_MAX_HEALTH = 100.0f;
    constexpr float TREE_DAMAGE_PER_HIT = 25.0f;
    constexpr int WOOD_PER_HIT = 1;
    constexpr int WOOD_ON_DESTROY = 5;
    constexpr float TREE_RAY_MAX_T = 200.0f; // ray-travel cap for tree picking, not the player's reach
    constexpr float PICKUP_SCATTER_RADIUS = 1.0f;

    // Closest entity with Transform + Bounds + Required... whose box the ray hits and that passes filter.
    template <typename... Required, typename Filter>
    entt::entity FindClosestAlongRay(entt::registry &registry, glm::vec3 rayOrigin, glm::vec3 rayDir, float maxDistance,
                                     Filter &&filter) {
        entt::entity closest = entt::null;
        float closestT = maxDistance;
        auto view = registry.view<TransformComponent, BoundsComponent, Required...>();
        for (auto entity : view) {
            if (!filter(entity)) continue;
            float t;
            if (RayMath::RayIntersectsBaseBox(rayOrigin, rayDir, view.template get<TransformComponent>(entity).Position,
                                              view.template get<BoundsComponent>(entity).halfExtents, t) &&
                t < closestT) {
                closestT = t;
                closest = entity;
            }
        }
        return closest;
    }

    constexpr auto AnyEntity = [](entt::entity) { return true; };

    // Drops an item stack on the ground, scattered slightly around pos
    void SpawnPickup(entt::registry &registry, ItemId item, int count, glm::vec3 pos, VulkanEngine *engine,
                     MeshRenderer *meshRenderer) {
        float angle = static_cast<float>(rand()) / RAND_MAX * glm::two_pi<float>();
        float dist = static_cast<float>(rand()) / RAND_MAX * PICKUP_SCATTER_RADIUS;

        auto pickup = registry.create();
        auto &transform = registry.emplace<TransformComponent>(pickup);
        transform.Position = pos + glm::vec3(cos(angle) * dist, 0.0f, sin(angle) * dist);
        transform.Scale = glm::vec3(0.3f);
        registry.emplace<PickupComponent>(pickup, PickupComponent{ item, count });
        if (auto mesh = ItemDatabase::GetWorldMesh(item, engine, meshRenderer)) registry.emplace<MeshComponent>(pickup, mesh);
    }

    // Moves up to `amount` of `item` from the player's inventory into the slot-less fuel buffer of
    // a furnace, generator or miner (they all share fuelBuffer/loadedFuelType).
    template <typename FuelComp>
    bool LoadFuel(FuelComp &machine, InventoryComponent &inventory, ItemId item, int amount) {
        if (machine.fuelBuffer > 0 && machine.loadedFuelType != item) return false; // one fuel type at a time
        for (auto &slot : inventory.slots) {
            if (slot.item != item || slot.count <= 0) continue;
            int take = std::min(slot.count, amount);
            slot.count -= take;
            if (slot.count == 0) slot.item = ItemId::None;
            machine.fuelBuffer += take;
            machine.loadedFuelType = item;
            return true;
        }
        return false;
    }
}

// ============================================================================
// Picking
// ============================================================================

entt::entity InteractionSystem::FindMachineAlongRay(entt::registry &registry, glm::vec3 rayOrigin, glm::vec3 rayDir, float maxDistance) {
    return FindClosestAlongRay<>(registry, rayOrigin, rayDir, maxDistance, [&](entt::entity e) {
        return registry.any_of<MachineInventoryComponent, MinerComponent, BeltComponent, InserterComponent,
                               PowerGeneratorComponent, PowerPoleComponent>(e);
    });
}

entt::entity InteractionSystem::FindSelectableAlongRay(entt::registry &registry, glm::vec3 rayOrigin, glm::vec3 rayDir, float maxDistance) {
    return FindClosestAlongRay<SelectableComponent>(registry, rayOrigin, rayDir, maxDistance, AnyEntity);
}

entt::entity InteractionSystem::FindDamageableAlongRay(entt::registry &registry, glm::vec3 rayOrigin, glm::vec3 rayDir, float maxDistance) {
    return FindClosestAlongRay<HealthComponent>(registry, rayOrigin, rayDir, maxDistance, AnyEntity);
}

entt::entity InteractionSystem::FindEnemyAlongRay(entt::registry &registry, glm::vec3 rayOrigin, glm::vec3 rayDir, float maxDistance,
                                                  uint8_t myTeam) {
    return FindClosestAlongRay<HealthComponent, TeamComponent>(
        registry, rayOrigin, rayDir, maxDistance, [&](entt::entity e) { return registry.get<TeamComponent>(e).team != myTeam; });
}

// ============================================================================
// Pickups and mining
// ============================================================================

entt::entity InteractionSystem::FindNearestPickup(entt::registry &registry, glm::vec3 playerPos, float range) {
    entt::entity closest = entt::null;
    float closestDist = range;
    auto view = registry.view<TransformComponent, PickupComponent>();
    for (auto entity : view) {
        float dist = glm::length(view.get<TransformComponent>(entity).Position - playerPos);
        if (dist <= closestDist) {
            closestDist = dist;
            closest = entity;
        }
    }
    return closest;
}

void InteractionSystem::CollectPickup(entt::registry &registry, entt::entity pickup, entt::entity player, AudioEventSystem *audio) {
    if (!registry.valid(pickup) || !registry.any_of<PickupComponent>(pickup)) return;

    auto &pickupComp = registry.get<PickupComponent>(pickup);
    int leftover = registry.get<InventoryComponent>(player).AddItem(pickupComp.item, pickupComp.count);
    if (leftover == 0) {
        registry.destroy(pickup);
        audio->Trigger(AudioEvent::OreCollected); // TODO: rename to a generic ItemPickup event
    } else {
        pickupComp.count = leftover; // partial pickup if inventory was nearly full
    }
}

bool InteractionSystem::TryMineAtCursor(entt::registry &registry, WorldState &world, entt::entity player, glm::vec3 rayOrigin,
                                        glm::vec3 rayDir, const TerrainSettings &terrainSettings, float maxRange,
                                        VulkanEngine *engine, MeshRenderer *meshRenderer, ChunkManager &chunkManager,
                                        const TreeRenderer &treeRenderer) {
    glm::vec3 playerPos = registry.get<TransformComponent>(player).Position;
    glm::vec3 groundHit = TerrainRaycast::RaycastToTerrain(rayOrigin, rayDir, terrainSettings);
    if (glm::length(groundHit - playerPos) > maxRange) return false;

    // Ore first: take one item straight into the inventory and deplete the cell
    if (auto deposit = OreDepositMap::GetDepositAt(groundHit.x, groundHit.z, terrainSettings.seed)) {
        int cellX = static_cast<int>(std::round(groundHit.x));
        int cellZ = static_cast<int>(std::round(groundHit.z));
        if (world.depletionMap.GetRemainingFraction(cellX, cellZ) > 0.0f) {
            registry.get<InventoryComponent>(player).AddItem(deposit->item, 1);
            world.depletionMap.Deplete(cellX, cellZ, 1.0f, deposit->amount);
            chunkManager.RequestChunkRegeneration(ChunkManager::WorldToChunkCoord(groundHit));
            return true;
        }
    }

    // Otherwise chop the tree under the cursor: wood drops each hit, a bigger drop when it falls
    auto tree = TreeRaycast::FindTreeAlongRay(rayOrigin, rayDir, groundHit, TREE_RAY_MAX_T, chunkManager, treeRenderer,
                                              world.removedTrees);
    if (!tree || glm::length(tree->basePos - playerPos) > maxRange) return false;

    world.treeHealth.Damage(tree->chunk, tree->gridIndex, TREE_DAMAGE_PER_HIT, TREE_MAX_HEALTH);
    int woodAmount = WOOD_PER_HIT;
    if (world.treeHealth.IsDead(tree->chunk, tree->gridIndex, TREE_MAX_HEALTH)) {
        woodAmount = WOOD_ON_DESTROY;
        world.removedTrees.MarkRemoved(tree->chunk, tree->gridIndex);
        world.treeHealth.Clear(tree->chunk, tree->gridIndex);
        chunkManager.RequestChunkRegeneration(tree->chunk);
    }
    SpawnPickup(registry, ItemId::Wood, woodAmount, tree->basePos, engine, meshRenderer);
    return true;
}

// ============================================================================
// Machines
// ============================================================================

bool InteractionSystem::TryFuelMachine(entt::registry &registry, entt::entity machine, entt::entity player, ItemId item, int amount) {
    if (!registry.valid(machine) || !FuelDatabase::IsFuel(item)) return false;
    auto &inventory = registry.get<InventoryComponent>(player);

    if (auto *furnace = registry.try_get<FurnaceComponent>(machine)) return LoadFuel(*furnace, inventory, item, amount);
    if (auto *generator = registry.try_get<PowerGeneratorComponent>(machine)) return LoadFuel(*generator, inventory, item, amount);
    if (auto *miner = registry.try_get<MinerComponent>(machine)) return LoadFuel(*miner, inventory, item, amount);
    return false;
}

bool InteractionSystem::TryInsertIntoMachine(entt::registry &registry, entt::entity machine, entt::entity player, ItemId item, int amount) {
    if (!registry.valid(machine) || !registry.any_of<MachineInventoryComponent>(machine)) return false;

    auto &machineInv = registry.get<MachineInventoryComponent>(machine);
    auto &inventory = registry.get<InventoryComponent>(player);

    for (auto &slot : inventory.slots) {
        if (slot.item != item || slot.count <= 0) continue;
        int take = std::min(slot.count, amount);

        int taken = 0;
        for (auto &inSlot : machineInv.inputs) {
            if ((inSlot.item == item || inSlot.item == ItemId::None) && inSlot.count < inSlot.capacity) {
                int toAdd = std::min(inSlot.capacity - inSlot.count, take - taken);
                inSlot.item = item;
                inSlot.count += toAdd;
                taken += toAdd;
                if (taken >= take) break;
            }
        }
        if (taken <= 0) return false; // machine's input was already full

        slot.count -= taken;
        if (slot.count == 0) slot.item = ItemId::None;
        return true;
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
        if (leftover < outSlot.count) {
            outSlot.count = leftover;
            if (outSlot.count == 0) outSlot.item = ItemId::None;
            collectedAny = true;
        }
    }
    return collectedAny;
}

bool InteractionSystem::TryCollectMinerOutput(entt::registry &registry, entt::entity minerEntity, entt::entity player) {
    if (!registry.valid(minerEntity) || !registry.any_of<MinerComponent>(minerEntity)) return false;

    auto &miner = registry.get<MinerComponent>(minerEntity);
    if (miner.outputBuffer <= 0) return false;

    // Whatever doesn't fit stays in the buffer
    miner.outputBuffer = registry.get<InventoryComponent>(player).AddItem(miner.outputItem, miner.outputBuffer);
    return true;
}

bool InteractionSystem::TryRotateMachine(entt::registry &registry, entt::entity target) {
    if (!registry.valid(target)) return false;

    glm::vec3 *facing = nullptr;
    if (auto *belt = registry.try_get<BeltComponent>(target))
        facing = &belt->direction;
    else if (auto *inserter = registry.try_get<InserterComponent>(target))
        facing = &inserter->facing;
    else
        return false; // only belts/inserters have a meaningful facing

    // Rotate 90 degrees: (x,z) -> (-z,x), same step the placement ghost uses
    *facing = glm::vec3(-facing->z, 0.0f, facing->x);
    float angle = atan2(facing->x, facing->z);
    registry.get<TransformComponent>(target).Rotation = glm::angleAxis(angle, glm::vec3(0, 1, 0));
    return true;
}

bool InteractionSystem::TryPickupMachine(entt::registry &registry, entt::entity target, entt::entity player,
                                         PlacementGrid &placementGrid, float gridSize) {
    if (!registry.valid(target)) return false;

    auto &inventory = registry.get<InventoryComponent>(player);
    ItemId machineItem = ItemId::None;

    // Work out what item this machine is, and return anything it was holding
    if (auto *miner = registry.try_get<MinerComponent>(target)) {
        if (miner->outputItem != ItemId::None && miner->outputBuffer > 0) inventory.AddItem(miner->outputItem, miner->outputBuffer);
        machineItem = ItemId::Miner;
    } else if (registry.any_of<FurnaceComponent>(target)) {
        machineItem = ItemId::Furnace;
    } else if (registry.any_of<AssemblerComponent>(target)) {
        machineItem = ItemId::Assembler;
    } else if (auto *belt = registry.try_get<BeltComponent>(target)) {
        for (auto &item : belt->leftLane.queue)
            if (item.item != ItemId::None) inventory.AddItem(item.item, 1);
        for (auto &item : belt->rightLane.queue)
            if (item.item != ItemId::None) inventory.AddItem(item.item, 1);
        machineItem = ItemId::Belt;
    } else if (auto *inserter = registry.try_get<InserterComponent>(target)) {
        if (inserter->holdingItem && inserter->heldItem != ItemId::None) inventory.AddItem(inserter->heldItem, 1);
        machineItem = ItemId::Inserter;
    } else if (registry.any_of<PowerGeneratorComponent>(target)) {
        machineItem = ItemId::CoalGenerator;
    } else if (registry.any_of<PowerPoleComponent>(target)) {
        machineItem = ItemId::PowerPole;
    } else {
        return false; // not a recognized machine type
    }

    // Furnace/assembler/generator input and output slots
    if (auto *inv = registry.try_get<MachineInventoryComponent>(target)) {
        for (auto &slot : inv->inputs)
            if (slot.item != ItemId::None) inventory.AddItem(slot.item, slot.count);
        for (auto &slot : inv->outputs)
            if (slot.item != ItemId::None) inventory.AddItem(slot.item, slot.count);
    }

    // Free the grid tiles this machine occupied
    glm::vec3 halfExtents = registry.any_of<BoundsComponent>(target) ? registry.get<BoundsComponent>(target).halfExtents : glm::vec3(0.5f);
    placementGrid.UnregisterArea(PlacementGrid::GetCoveredCells(registry.get<TransformComponent>(target).Position, halfExtents, gridSize));

    inventory.AddItem(machineItem, 1);
    registry.destroy(target);
    return true;
}
