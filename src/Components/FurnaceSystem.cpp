// FurnaceSystem.cpp
#include "FurnaceSystem.h"
#include "FurnaceComponent.h"
#include "../Game/FurnaceRecipeDatabase.h"
#include "MachineInventoryComponent.h"
#include "../Game/FuelDatabase.h"

void FurnaceSystem::Update(entt::registry &registry, float deltaTime) {
    auto view = registry.view<FurnaceComponent, MachineInventoryComponent>();
    for (auto entity : view) {
        auto &furnace = view.get<FurnaceComponent>(entity);
        auto &inv = view.get<MachineInventoryComponent>(entity);

        if (!furnace.isCooking) {
            // Need fuel available before starting a new cook cycle
            if (furnace.fuelRemaining <= 0.0f) {
                if (furnace.fuelBuffer <= 0) continue; // no fuel loaded — idle
                const FuelDef* fuelDef = FuelDatabase::TryGet(furnace.loadedFuelType);
                if (!fuelDef) continue;   // shouldn't happen, but guards against bad state

                furnace.fuelBuffer--;
                furnace.fuelRemaining = fuelDef->burnTime;
                if (furnace.fuelBuffer == 0) furnace.loadedFuelType = ItemId::None;
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
            furnace.cookTimer -= deltaTime;
            furnace.fuelRemaining -= deltaTime; // fuel burns down while actively cooking

            if (furnace.cookTimer <= 0.0f) {
                MachineInventoryComponent::AddToSlots(inv.outputs, furnace.currentOutput, 1);
                furnace.isCooking = false;
                furnace.currentOutput = ItemId::None;
            }
        }
    }
}