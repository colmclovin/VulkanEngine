// FurnaceComponent.h
#pragma once
#include "../Game/ItemDatabase.h"

struct FurnaceComponent {
    float cookTimer = 0.0f;
    bool isCooking = false;
    ItemId currentOutput = ItemId::None;

    ItemId loadedFuelType = ItemId::None;   // what's actually in the buffer right now
    int fuelBuffer = 0;
    float fuelRemaining = 0.0f;

    float powerUsage = 5.0f; // NEW — power units consumed per second while running on power
    bool runningOnPower = false; // NEW — informational, tracks which mode this cook cycle used
};