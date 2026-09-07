#pragma once
#include "../Game/ItemDatabase.h"
#include <vector>

struct MachineSlot {
    ItemId item = ItemId::None;
    int count = 0;
    int capacity = 100;
};

struct MachineInventoryComponent {
    std::vector<MachineSlot> inputs;
    std::vector<MachineSlot> outputs;

    // Returns leftover that didn't fit
    static int AddToSlots(std::vector<MachineSlot> &slots, ItemId item, int count) {
        for (auto &slot : slots) {
            if (slot.item == item && slot.count < slot.capacity) {
                int space = slot.capacity - slot.count;
                int add = std::min(space, count);
                slot.count += add;
                count -= add;
                if (count == 0) return 0;
            }
        }
        for (auto &slot : slots) {
            if (slot.item == ItemId::None) {
                int add = std::min(slot.capacity, count);
                slot.item = item;
                slot.count = add;
                count -= add;
                if (count == 0) return 0;
            }
        }
        return count;
    }

    static bool RemoveFromSlots(std::vector<MachineSlot> &slots, ItemId item, int count) {
        int available = 0;
        for (auto &slot : slots)
            if (slot.item == item) available += slot.count;
        if (available < count) return false;

        int remaining = count;
        for (auto &slot : slots) {
            if (slot.item == item && remaining > 0) {
                int take = std::min(slot.count, remaining);
                slot.count -= take;
                remaining -= take;
                if (slot.count == 0) slot.item = ItemId::None;
            }
        }
        return true;
    }
};