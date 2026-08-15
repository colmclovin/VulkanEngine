#pragma once
#include "../Game/ItemDatabase.h"
#include <vector>

struct ItemStack {
    ItemId item = ItemId::None;
    int count = 0;
};

struct InventoryComponent {
    std::vector<ItemStack> slots;
    int capacity = 20;

    InventoryComponent() { slots.resize(capacity); }

    // Returns leftover count that didn't fit
    int AddItem(ItemId item, int count) {
        const ItemDef &def = ItemDatabase::Get(item);

        // Fill existing stacks first
        for (auto &slot : slots) {
            if (slot.item == item && slot.count < def.maxStackSize) {
                int space = def.maxStackSize - slot.count;
                int toAdd = std::min(space, count);
                slot.count += toAdd;
                count -= toAdd;
                if (count == 0) return 0;
            }
        }
        // Then empty slots
        for (auto &slot : slots) {
            if (slot.item == ItemId::None) {
                int toAdd = std::min(def.maxStackSize, count);
                slot.item = item;
                slot.count = toAdd;
                count -= toAdd;
                if (count == 0) return 0;
            }
        }
        return count; // whatever didn't fit
    }
};