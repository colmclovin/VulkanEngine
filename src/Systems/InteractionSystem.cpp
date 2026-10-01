// InteractionSystem.cpp
#include "InteractionSystem.h"
#include "../Audio/AudioEventSystem.h"
#include "../Components/Components.h"
#include <limits>
#include "../World/TerrainRaycast.h"
#include <iostream>
#include "../World/PlacementGrid.h"
#include "../Game/FuelDatabase.h"
#include "../Engine/VulkanEngine.h"
#include "../World/DepletionMap.h"
#include "../Game/OreDepositMap.h"
#include "../Game/OreDatabase.h"
#include "../World/TreePlacement.h"
#include "../World/TreeHealthMap.h"
#include "../Renderer/TreeRenderer.h"
#include "../Rendering/TreeInstanceBufferPool.h"

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

void InteractionSystem::Mine(entt::registry &registry, entt::entity target, entt::entity player, AudioEventSystem *audio,
							 VulkanEngine *engine, MeshRenderer *meshRenderer, PlacementGrid &placementGrid, RemovedTreesMap &removedTreesMap, float cellSize) {
	if (!registry.valid(target) || !registry.any_of<HarvestableComponent>(target)) return;
	std::cout << "Mine() registry address: " << &registry << std::endl;
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
		std::cout << "Pickup created, storage count: " << registry.storage<entt::entity>().size() << std::endl;
		size_t liveCount3 = 0;
		for (auto entity : registry.storage<entt::entity>()) {
			liveCount3++;
		}
		std::cout << "  [verified] live entity count: " << liveCount3 << std::endl;
		auto &pickupTransform = registry.emplace<TransformComponent>(pickupEntity);
		pickupTransform.Position = targetTransform.Position + offset;
		pickupTransform.Scale = glm::vec3(0.3f); 
		registry.emplace<PickupComponent>(pickupEntity, PickupComponent{ harvest.yieldItem, harvest.yieldOnDestroy });
		// TODO: give it a small mesh (a dropped-item model) via MeshComponent once you have one
		auto dropMesh = ItemDatabase::GetWorldMesh(harvest.yieldItem, engine, meshRenderer);
		if (dropMesh) {
			registry.emplace<MeshComponent>(pickupEntity, dropMesh);
		}
	if (harvest.health <= 0.0f) {
		GridCoord coord = PlacementGrid::WorldToGrid(targetTransform.Position, cellSize);
		placementGrid.Unregister(coord);
        auto &origin = registry.get<TreeOriginComponent>(target);
        removedTreesMap.MarkRemoved(origin.chunkCoord, origin.candidateIndex); // NEW
		std::cout << "About to destroy tree, target valid? " << registry.valid(target) << std::endl;
		registry.destroy(target);
		std::cout << "After destroy, target valid? " << registry.valid(target) << std::endl;
		std::cout << "Tree destroyed, storage count: " << registry.storage<entt::entity>().size() << std::endl;
		size_t liveCount2 = 0;
		for (auto entity : registry.storage<entt::entity>()) {
			liveCount2++;
		}
		std::cout << "  [verified] live entity count: " << liveCount2 << std::endl;

	}
}

void InteractionSystem::CollectPickup(entt::registry &registry, entt::entity pickup, entt::entity player, AudioEventSystem *audio) {
	if (!registry.valid(pickup) || !registry.any_of<PickupComponent>(pickup)) return;

	auto &pickupComp = registry.get<PickupComponent>(pickup);
	auto &inventory = registry.get<InventoryComponent>(player);

	int leftover = inventory.AddItem(pickupComp.item, pickupComp.count);
	if (leftover == 0) {
		// Fully picked up
		std::cout << "About to destroy pickup, pickup valid? " << registry.valid(pickup) << std::endl;
		registry.destroy(pickup);
		std::cout << "After destroy, pickup valid? " << registry.valid(pickup) << std::endl;
		std::cout << "Pickup collected, storage count: " << registry.storage<entt::entity>().size() << std::endl;

		size_t liveCount = 0;
		for (auto entity : registry.storage<entt::entity>()) {
			liveCount++;
		}
		std::cout << "  [verified] live entity count: " << liveCount << std::endl;
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

bool InteractionSystem::TryMineAtCursor(entt::registry& registry, DepletionMap& depletionMap, entt::entity player,
	glm::vec3 rayOrigin, glm::vec3 rayDir, const TerrainSettings& terrainSettings,
	float maxRange, AudioEventSystem* audio, VulkanEngine* engine, MeshRenderer* meshRenderer,
	PlacementGrid& placementGrid, RemovedTreesMap& removedTreesMap, ChunkManager& chunkManager,
	TreeHealthMap& treeHealthMap, TreeRenderer& treeRenderer) {
	auto& playerTransform = registry.get<TransformComponent>(player);

	glm::vec3 groundHit = TerrainRaycast::RaycastToTerrain(rayOrigin, rayDir, terrainSettings);
	float groundDist = glm::length(groundHit - playerTransform.Position);

	if (groundDist > maxRange) return false;

	// --- Try ore first (unchanged, already working) ---
	auto deposit = OreDepositMap::GetDepositAt(groundHit.x, groundHit.z, terrainSettings.seed);
	if (deposit) {
		int cellX = static_cast<int>(std::round(groundHit.x));
		int cellZ = static_cast<int>(std::round(groundHit.z));
		float remaining = depletionMap.GetRemainingFraction(cellX, cellZ);

		if (remaining > 0.0f) {
			auto& inventory = registry.get<InventoryComponent>(player);
			inventory.AddItem(deposit->item, 1);
			depletionMap.Deplete(cellX, cellZ, 1.0f, deposit->amount);
			//audio->Trigger(AudioEvent::OreCollected);

			ChunkCoord affectedChunk = ChunkManager::WorldToChunkCoord(glm::vec3(groundHit.x, 0, groundHit.z));
			chunkManager.RequestChunkRegeneration(affectedChunk);
			return true;
		}
	}

	// --- Tree chopping: search a 3x3 chunk neighborhood, not just the ground-hit chunk ---
	const float TREE_MAX_HEALTH = 100.0f;
	const float TREE_DAMAGE_PER_HIT = 25.0f;
	const int WOOD_PER_HIT = 1;
	const int WOOD_ON_DESTROY = 5;

	ChunkCoord centerCoord = ChunkManager::WorldToChunkCoord(groundHit);

	int bestIndex = -1;
	float bestT = 200.0f;   // generous ray-travel cap, not the player-interaction range
	ChunkCoord bestCoord{};
	std::vector<TreeInstanceReadback> bestInstances;

	for (int dz = -1; dz <= 1; dz++) {
		for (int dx = -1; dx <= 1; dx++) {
			ChunkCoord checkCoord{ centerCoord.x + dx, centerCoord.z + dz };
			auto treeSlotOpt = chunkManager.GetTreeSlotForChunk(checkCoord);
			if (!treeSlotOpt.has_value()) continue;

			auto treeInstances = treeRenderer.ReadBackChunkInstances(*treeSlotOpt);
			float localT = bestT;
			int localIndex = FindTreeAlongRayWithT(treeInstances, removedTreesMap, checkCoord, rayOrigin, rayDir, bestT, localT);

			if (localIndex >= 0 && localT < bestT) {
				bestT = localT;
				bestIndex = localIndex;
				bestCoord = checkCoord;
				bestInstances = treeInstances;
			}
		}
	}

	if (bestIndex >= 0) {
		glm::vec3 treeBasePos = bestInstances[bestIndex].position;
		float playerToTreeDist = glm::length(treeBasePos - playerTransform.Position);

		if (playerToTreeDist <= maxRange) {
			int gridIndex = bestInstances[bestIndex].gridIndex;

			treeHealthMap.Damage(bestCoord, gridIndex, TREE_DAMAGE_PER_HIT, TREE_MAX_HEALTH);
			//audio->Trigger(AudioEvent::TreeChopped);

			int woodAmount = WOOD_PER_HIT;
			bool destroyed = treeHealthMap.IsDead(bestCoord, gridIndex, TREE_MAX_HEALTH);
			if (destroyed) {
				woodAmount = WOOD_ON_DESTROY;
				removedTreesMap.MarkRemoved(bestCoord, gridIndex);
				treeHealthMap.Clear(bestCoord, gridIndex);
				chunkManager.RequestChunkRegeneration(bestCoord);
			}

			float scatterRadius = 1.0f;
			float angle = static_cast<float>(rand()) / RAND_MAX * glm::two_pi<float>();
			float scatterDist = static_cast<float>(rand()) / RAND_MAX * scatterRadius;
			glm::vec3 offset(cos(angle) * scatterDist, 0.0f, sin(angle) * scatterDist);

			auto pickupEntity = registry.create();
			auto& pickupTransform = registry.emplace<TransformComponent>(pickupEntity);
			pickupTransform.Position = treeBasePos + offset;
			pickupTransform.Scale = glm::vec3(0.3f);
			registry.emplace<PickupComponent>(pickupEntity, PickupComponent{ ItemId::Wood, woodAmount });
			auto dropMesh = ItemDatabase::GetWorldMesh(ItemId::Wood, engine, meshRenderer);
			if (dropMesh) registry.emplace<MeshComponent>(pickupEntity, dropMesh);

			return true;
		}
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
	if (!FuelDatabase::IsFuel(selectedItem)) return false;   // CHANGED — check the database, not a fixed item

	auto& miner = registry.get<MinerComponent>(minerEntity);

	// Simplification: only allow loading one fuel TYPE at a time — refuse if a different fuel is already loaded and buffer isn't empty
	if (miner.fuelBuffer > 0 && miner.loadedFuelType != selectedItem) {
		return false;   // miner is currently loaded with a different fuel type
	}

	auto& inventory = registry.get<InventoryComponent>(player);
	for (auto& slot : inventory.slots) {
		if (slot.item == selectedItem && slot.count > 0) {
			int take = std::min(slot.count, amount);
			slot.count -= take;
			if (slot.count == 0) slot.item = ItemId::None;

			miner.fuelBuffer += take;
			miner.loadedFuelType = selectedItem;
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
		bool isMachine = registry.any_of<MachineInventoryComponent, MinerComponent, BeltComponent, InserterComponent,
										 PowerGeneratorComponent, PowerPoleComponent>(entity);
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

			int actuallyTaken = 0;
			for (auto &inSlot : machineInv.inputs) {
				if ((inSlot.item == item || inSlot.item == ItemId::None) && inSlot.count < inSlot.capacity) {
					int room = inSlot.capacity - inSlot.count;
					int toAdd = std::min(room, take - actuallyTaken);
					inSlot.item = item;
					inSlot.count += toAdd;
					actuallyTaken += toAdd;
					if (actuallyTaken >= take) break;
				}
			}

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
bool InteractionSystem::TryFuelFurnace(entt::registry& registry, entt::entity furnaceEntity, entt::entity player, ItemId selectedItem, int amount) {
	if (!registry.valid(furnaceEntity) || !registry.any_of<FurnaceComponent>(furnaceEntity)) return false;
	if (!FuelDatabase::IsFuel(selectedItem)) return false;   // CHANGED — check the database, not a fixed item

	auto& furnace = registry.get<FurnaceComponent>(furnaceEntity);

	// Simplification: only allow loading one fuel TYPE at a time — refuse if a different fuel is already loaded and buffer isn't empty
	if (furnace.fuelBuffer > 0 && furnace.loadedFuelType != selectedItem) {
		return false;   // furnace is currently loaded with a different fuel type
	}

	auto& inventory = registry.get<InventoryComponent>(player);
	for (auto& slot : inventory.slots) {
		if (slot.item == selectedItem && slot.count > 0) {
			int take = std::min(slot.count, amount);
			slot.count -= take;
			if (slot.count == 0) slot.item = ItemId::None;

			furnace.fuelBuffer += take;
			furnace.loadedFuelType = selectedItem;
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
// InteractionSystem.cpp
bool InteractionSystem::TryFuelGenerator(entt::registry &registry, entt::entity generatorEntity, entt::entity player, ItemId selectedItem, int amount) {
	if (!registry.valid(generatorEntity) || !registry.any_of<PowerGeneratorComponent>(generatorEntity)) return false;
	if (!FuelDatabase::IsFuel(selectedItem)) return false;

	auto &gen = registry.get<PowerGeneratorComponent>(generatorEntity);
	if (gen.fuelBuffer > 0 && gen.loadedFuelType != selectedItem) return false;

	auto &inventory = registry.get<InventoryComponent>(player);
	for (auto &slot : inventory.slots) {
		if (slot.item == selectedItem && slot.count > 0) {
			int take = std::min(slot.count, amount);
			slot.count -= take;
			if (slot.count == 0) slot.item = ItemId::None;

			gen.fuelBuffer += take;
			gen.loadedFuelType = selectedItem;
			return true;
		}
	}
	return false;
}

bool TryChopTreeAtCursor(glm::vec3 cursorWorldPos, RemovedTreesMap& removedTreesMap, ChunkManager& chunkManager,
	const TerrainSettings& settings, InventoryComponent& inventory, AudioEngine* audio) {
	ChunkCoord coord = ChunkManager::WorldToChunkCoord(cursorWorldPos);
	auto candidates = TreePlacement::GetTreesInChunk(coord.x, coord.z, ChunkManager::CHUNK_WORLD_SIZE, settings);

	for (int i = 0; i < static_cast<int>(candidates.size()); i++) {
		if (removedTreesMap.IsRemoved(coord, i)) continue;
		float dist = glm::length(candidates[i].position - cursorWorldPos);
		if (dist < 1.5f) {
			removedTreesMap.MarkRemoved(coord, i);
			inventory.AddItem(ItemId::Wood, 5);
			//if (audio) audio->Trigger(AudioEvent::WoodCollected);   // adjust to your actual event enum
			chunkManager.RequestChunkRegeneration(coord);
			return true;
		}
	}
	return false;
}

// InteractionSystem.cpp
int InteractionSystem::FindTreeAlongRay(const std::vector<TreeInstanceReadback>& treeInstances,
	const RemovedTreesMap& removedTreesMap, ChunkCoord coord,
	glm::vec3 rayOrigin, glm::vec3 rayDir, float maxDistance) {
	int closestIndex = -1;
	float closestT = maxDistance;

	for (int i = 0; i < static_cast<int>(treeInstances.size()); i++) {
		int gridIndex = treeInstances[i].gridIndex;
		if (removedTreesMap.IsRemoved(coord, gridIndex)) continue;

		glm::vec3 base = treeInstances[i].position;
		glm::vec3 boxMin = base + glm::vec3(-0.6f, 0.0f, -0.6f);
		glm::vec3 boxMax = base + glm::vec3(0.6f, 3.0f, 0.6f);

		float t;
		bool hit = RayIntersectsAABB(rayOrigin, rayDir, boxMin, boxMax, t);
		std::cout << "  tree " << i << " base=(" << base.x << "," << base.y << "," << base.z
			<< ") hit=" << hit << " t=" << (hit ? t : -1.0f) << " closestT=" << closestT << std::endl;

		if (hit && t < closestT) {
			closestT = t;
			closestIndex = i;
		}
	}
	std::cout << "rayOrigin=(" << rayOrigin.x << "," << rayOrigin.y << "," << rayOrigin.z
		<< ") rayDir=(" << rayDir.x << "," << rayDir.y << "," << rayDir.z << ") length=" << glm::length(rayDir) << std::endl;
	return closestIndex;
}
int InteractionSystem::FindTreeAlongRayWithT(const std::vector<TreeInstanceReadback>& treeInstances,
	const RemovedTreesMap& removedTreesMap, ChunkCoord coord,
	glm::vec3 rayOrigin, glm::vec3 rayDir, float maxDistance, float& outClosestT) {
	int closestIndex = -1;
	float closestT = maxDistance;

	for (int i = 0; i < static_cast<int>(treeInstances.size()); i++) {
		int gridIndex = treeInstances[i].gridIndex;
		if (removedTreesMap.IsRemoved(coord, gridIndex)) continue;

		glm::vec3 base = treeInstances[i].position;
		glm::vec3 boxMin = base + glm::vec3(-0.6f, 0.0f, -0.6f);
		glm::vec3 boxMax = base + glm::vec3(0.6f, 3.0f, 0.6f);

		float t;
		if (RayIntersectsAABB(rayOrigin, rayDir, boxMin, boxMax, t) && t < closestT) {
			closestT = t;
			closestIndex = i;
		}
	}

	outClosestT = closestT;
	return closestIndex;
}