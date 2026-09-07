// AssemblerSystem.cpp
#include "AssemblerSystem.h"
#include "AssemblerComponent.h"
#include "MachineInventoryComponent.h"
#include "../Game/RecipeDatabase.h"

void AssemblerSystem::Update(entt::registry &registry, float deltaTime) {
    auto view = registry.view<AssemblerComponent, MachineInventoryComponent>();
    for (auto entity : view) {
        auto &assembler = view.get<AssemblerComponent>(entity);
        auto &inv = view.get<MachineInventoryComponent>(entity);

        if (assembler.selectedRecipeIndex < 0) continue;
        const auto &recipes = RecipeDatabase::GetAll();
        if (assembler.selectedRecipeIndex >= static_cast<int>(recipes.size())) continue;
        const Recipe &recipe = recipes[assembler.selectedRecipeIndex];

        if (!assembler.isCrafting) {
            bool hasAllInputs = true;
            for (auto &req : recipe.inputs) {
                int have = 0;
                for (auto &slot : inv.inputs)
                    if (slot.item == req.item) have += slot.count;
                if (have < req.count) {
                    hasAllInputs = false;
                    break;
                }
            }
            if (!hasAllInputs) continue;

            bool hasRoom = true;
            for (auto &out : recipe.outputs) {
                bool found = false;
                for (auto &slot : inv.outputs) {
                    if ((slot.item == out.item || slot.item == ItemId::None) && slot.count < slot.capacity) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    hasRoom = false;
                    break;
                }
            }
            if (!hasRoom) continue;

            for (auto &req : recipe.inputs) {
                MachineInventoryComponent::RemoveFromSlots(inv.inputs, req.item, req.count);
            }
            assembler.isCrafting = true;
            assembler.craftTimer = recipe.craftTime;
        } else {
            assembler.craftTimer -= deltaTime;
            if (assembler.craftTimer <= 0.0f) {
                for (auto &out : recipe.outputs) {
                    MachineInventoryComponent::AddToSlots(inv.outputs, out.item, out.count);
                }
                assembler.isCrafting = false;
            }
        }
    }
}