#pragma once
#include "../Game/ItemDatabase.h"
struct PowerGeneratorComponent {
    float powerOutput = 100.0f; // arbitrary units
    int fuelBuffer = 0;
    ItemId loadedFuelType = ItemId::None;
    float fuelRemaining = 0.0f;
};

struct PowerPoleComponent {
    float radius = 8.0f; // world units
};

struct PowerConsumerComponent {
    float powerRequired = 5.0f; // should match FurnaceComponent::powerUsage for this entity
    bool isPowered = false; // updated each frame by PowerSystem
};