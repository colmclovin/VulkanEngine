// AssemblerSystem.cpp
#include "AssemblerSystem.h"
#include "../Components/AssemblerComponent.h"
#include "../Components/MachineInventoryComponent.h"
#include "../Game/RecipeDatabase.h"
#include "../Components/PowerComponent.h"
#include "../Components/TransformComponent.h"
#include "../Components/TeamComponent.h"
#include "../Game/UnitDatabase.h"
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <iostream>
void AssemblerSystem::Update(entt::registry &registry, float deltaTime, std::vector<SpawnRequest> &spawns, int unitBudget) {
    auto view = registry.view<AssemblerComponent, MachineInventoryComponent>();
	for (auto entity : view) {
		auto &assembler = view.get<AssemblerComponent>(entity);
		auto &inv = view.get<MachineInventoryComponent>(entity);

		bool hasPower = false;
		if (registry.any_of<PowerConsumerComponent>(entity)) {
			hasPower = registry.get<PowerConsumerComponent>(entity).isPowered;
		}
		assembler.runningOnPower = hasPower;

		if (registry.any_of<PowerConsumerComponent>(entity)) {
			auto &consumer = registry.get<PowerConsumerComponent>(entity);
			bool couldStartCrafting = false;
			if (!assembler.isCrafting && assembler.selectedRecipeIndex >= 0) {
				// reuse your existing hasAllInputs check, or a simplified version
				couldStartCrafting = true; // simplify: assume wants power if a recipe is selected at all; refine later if needed
			}
			consumer.wantsPower = assembler.isCrafting || couldStartCrafting;
		}



		if (assembler.selectedRecipeIndex < 0) continue;
		const auto &recipes = RecipeDatabase::GetAll();
		if (assembler.selectedRecipeIndex >= static_cast<int>(recipes.size())) continue;
		const Recipe &recipe = recipes[assembler.selectedRecipeIndex];

		if (!assembler.isCrafting) {
			if (!hasPower) continue; // ADD — can't START a new craft cycle without power

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
                if (assembler.deployUnits && UnitDatabase::TryGet(out.item)) continue; // deployed, so no slot needed
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
            if (!hasPower) continue;

            assembler.craftTimer -= deltaTime;
            if (assembler.craftTimer <= 0.0f) {
                // If we'd deploy units but the cap is reached, wait here with the finished craft

                bool wantsDeploy = false;
                if (assembler.deployUnits)
                    for (auto &out : recipe.outputs)
                        if (UnitDatabase::TryGet(out.item)) wantsDeploy = true;
                if (wantsDeploy && unitBudget <= 0) {
                    assembler.craftTimer = 0.0f;
                    continue;
                }

                glm::vec3 pos = registry.get<TransformComponent>(entity).Position;
                uint8_t team = 0;
                if (auto *tc = registry.try_get<TeamComponent>(entity)) team = tc->team;

                for (auto &out : recipe.outputs) {
                    if (assembler.deployUnits && UnitDatabase::TryGet(out.item)) {
                        static int counter = 0;
                        for (int n = 0; n < out.count && unitBudget > 0; n++, unitBudget--) {
                            float angle = (counter++ % 8) * (glm::two_pi<float>() / 8.0f);
                            spawns.push_back({ out.item,
                                               pos + glm::vec3(std::cos(angle) * 4.0f, 0.0f, std::sin(angle) * 4.0f),
                                               team });
                        }
                        std::cout << "craft done: deploy=" << assembler.deployUnits
                                  << " wantsDeploy=" << wantsDeploy << " budget=" << unitBudget << std::endl;
                    } else {
                        MachineInventoryComponent::AddToSlots(inv.outputs, out.item, out.count);
                    }
                }
                assembler.isCrafting = false;
            }
        }
	}
}
