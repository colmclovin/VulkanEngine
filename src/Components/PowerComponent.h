#pragma once
#include "../Game/ItemDatabase.h"
#include "../Helpers/GlmSerialization.h"



struct PowerGeneratorComponent {
    float powerOutput = 100.0f; // arbitrary units
    int fuelBuffer = 0;
    ItemId loadedFuelType = ItemId::None;
    float fuelRemaining = 0.0f;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(PowerGeneratorComponent, powerOutput, fuelBuffer, loadedFuelType, fuelRemaining)
};

struct PowerPoleComponent {
    float radius = 8.0f; // world units
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(PowerPoleComponent, radius)
};

struct PowerConsumerComponent {
    float powerRequired = 5.0f; // should match FurnaceComponent::powerUsage for this entity
    bool isPowered = false; // updated each frame by PowerSystem
    bool wantsPower = false;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(PowerConsumerComponent, powerRequired, isPowered, wantsPower)
};