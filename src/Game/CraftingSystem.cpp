// CraftingSystem.cpp
#include "CraftingSystem.h"
#include "../Components/Components.h"

bool CraftingSystem::CanCraft(entt::registry& registry, entt::entity actor, const Recipe& recipe) {
    auto& inventory = registry.get<InventoryComponent>(actor);

    for (auto& input : recipe.inputs) {
        int have = 0;
        for (auto& slot : inventory.slots) {
            if (slot.item == input.item) have += slot.count;
        }
        if (have < input.count) return false;
    }
    return true;
}

bool CraftingSystem::TryCraft(entt::registry& registry, entt::entity actor, const Recipe& recipe) {
    if (!CanCraft(registry, actor, recipe)) return false;

    auto& inventory = registry.get<InventoryComponent>(actor);

    // Consume inputs
    for (auto& input : recipe.inputs) {
        int remaining = input.count;
        for (auto& slot : inventory.slots) {
            if (slot.item == input.item && remaining > 0) {
                int take = std::min(slot.count, remaining);
                slot.count -= take;
                remaining -= take;
                if (slot.count == 0) slot.item = ItemId::None;
            }
        }
    }

    // Grant outputs
    for (auto& output : recipe.outputs) {
        inventory.AddItem(output.item, output.count);
    }

    return true;
}