// FurnaceSystem.cpp
#include "FurnaceSystem.h"
#include "../Components/FurnaceComponent.h"
#include "../Game/FurnaceRecipeDatabase.h"
#include "../Components/MachineInventoryComponent.h"
#include "../Game/FuelDatabase.h"
#include "../Components/PowerComponent.h"
#include "../Components/Components.h"
#include <iostream>

void FurnaceSystem::Update(entt::registry &registry, float deltaTime) {
	auto view = registry.view<FurnaceComponent, MachineInventoryComponent>();
	for (auto entity : view) {
		auto &furnace = view.get<FurnaceComponent>(entity);
		auto &inv = view.get<MachineInventoryComponent>(entity);

		bool hasPower = false;
		if (registry.any_of<PowerConsumerComponent>(entity)) {
			hasPower = registry.get<PowerConsumerComponent>(entity).isPowered;
		}

		if (registry.any_of<PowerConsumerComponent>(entity)) {
			auto &consumer = registry.get<PowerConsumerComponent>(entity);
			bool couldStartCooking = false;
			if (!furnace.isCooking) {
				for (auto &inSlot : inv.inputs) {
					if (inSlot.item != ItemId::None && FurnaceRecipeDatabase::TryGet(inSlot.item)) {
						couldStartCooking = true;
						break;
					}
				}
			}
			consumer.wantsPower = furnace.isCooking || couldStartCooking;
		}

		if (registry.any_of<PointLightComponent>(entity)) {
			auto &light = registry.get<PointLightComponent>(entity);
			light.active = furnace.isCooking;
			//std::cout << "Furnace light active: " << light.active << " (isCooking: " << furnace.isCooking << ")" << std::endl;

		}

		if (!furnace.isCooking) {
			// Need fuel available before starting a new cook cycle

			if (hasPower) {
				furnace.runningOnPower = true;
			} else {
				furnace.runningOnPower = false;

				if (furnace.fuelRemaining <= 0.0f) {
					if (furnace.fuelBuffer <= 0) continue; // no fuel loaded — idle
					const FuelDef *fuelDef = FuelDatabase::TryGet(furnace.loadedFuelType);
					if (!fuelDef) continue; // shouldn't happen, but guards against bad state

					furnace.fuelBuffer--;
					furnace.fuelRemaining = fuelDef->burnTime;
					if (furnace.fuelBuffer == 0) furnace.loadedFuelType = ItemId::None;
				}
			}

			for (auto &inSlot : inv.inputs) {
				if (inSlot.item == ItemId::None || inSlot.count <= 0) continue;
				if (FuelDatabase::IsFuel(inSlot.item)) continue; // don't try to "cook" the fuel itself

				const FurnaceRecipe *recipe = FurnaceRecipeDatabase::TryGet(inSlot.item);
				if (!recipe) continue;

				bool hasRoom = false;
				for (auto &outSlot : inv.outputs) {
					if ((outSlot.item == recipe->output || outSlot.item == ItemId::None) && outSlot.count < outSlot.capacity) {
						hasRoom = true;
						break;
					}
				}
				if (!hasRoom) continue;

				inSlot.count--;
				if (inSlot.count == 0) inSlot.item = ItemId::None;

				furnace.isCooking = true;

				furnace.cookTimer = recipe->cookTime;
				furnace.currentOutput = recipe->output;
				break;
			}
		} else {

			// Currently cooking — if running on power, power must stay available or the furnace stalls
			if (furnace.runningOnPower && !hasPower) {
				continue; // lost power mid-cook — pause (don't lose progress, just wait)
			}
			furnace.cookTimer -= deltaTime;
			if (!furnace.runningOnPower) {
				furnace.fuelRemaining -= deltaTime; // only burn fuel if actually using fuel this cycle
			}
			if (furnace.cookTimer <= 0.0f) {
				MachineInventoryComponent::AddToSlots(inv.outputs, furnace.currentOutput, 1);
				furnace.isCooking = false;
				furnace.currentOutput = ItemId::None;
			}
		}
	}
}
