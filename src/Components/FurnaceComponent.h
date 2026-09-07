// FurnaceComponent.h
#pragma once
#include "../Game/ItemDatabase.h"

struct FurnaceComponent {
    float cookTimer = 0.0f;
    bool isCooking = false;
    ItemId currentOutput = ItemId::None; // what this cook cycle will produce
};