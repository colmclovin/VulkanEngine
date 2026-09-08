// FurnaceComponent.h
#pragma once
#include "../Game/ItemDatabase.h"

struct FurnaceComponent {
    float cookTimer = 0.0f;
    bool isCooking = false;
    ItemId currentOutput = ItemId::None;

    ItemId fuelItem = ItemId::Wood;
    int fuelBuffer = 0;
    float fuelBurnTime = 5.0f; // seconds of cook-time this furnace can run per fuel item consumed
    float fuelRemaining = 0.0f;
};