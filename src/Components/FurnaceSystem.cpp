// FurnaceSystem.cpp
#include "FurnaceSystem.h"
#include "FurnaceComponent.h"
#include "../Game/FurnaceRecipeDatabase.h"
#include "MachineInventoryComponent.h"

void FurnaceSystem::Update(entt::registry &registry, float deltaTime) {
    auto view = registry.view<FurnaceComponent, MachineInventoryComponent>();
    for (auto entity : view) {
        auto &furnace = view.get<FurnaceComponent>(entity);
        auto &inv = view.get<MachineInventoryComponent>(entity);

        if (!furnace.isCooking) {
            for (auto &inSlot : inv.inputs) {
                if (inSlot.item == ItemId::None || inSlot.count <= 0) continue;
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
            if (furnace.cookTimer <= 0.0f) {
                MachineInventoryComponent::AddToSlots(inv.outputs, furnace.currentOutput, 1);
                furnace.isCooking = false;
                furnace.currentOutput = ItemId::None;
            }
        }
    }
}