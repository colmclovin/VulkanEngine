#pragma once
#include "../Game/ItemDatabase.h"
#include "../Helpers/GlmSerialization.h"  


struct MinerComponent {
    float collectionRadius = 3.0f; // how far around the miner it reaches into the ResourceMap
    float extractionRate = 1.0f; // resource units extracted per second, while fueled
    float outputInterval = 1.0f; // seconds between producing one item into its output buffer
    float outputTimer = 0.0f; // internal countdown

    ItemId loadedFuelType = ItemId::None;   // what's actually in the buffer right now
    int fuelBuffer = 0; // how many fuel items currently loaded
    float fuelRemaining = 0.0f; // seconds left on the currently-burning fuel item

    ItemId outputItem = ItemId::None; // set once, based on what resource is under it
    int outputBuffer = 0; // items waiting to be collected/transported
    int outputBufferCapacity = 50;

        float powerUsage = 5.0f; // NEW — power units consumed per second while running on power
    bool runningOnPower = false; // NEW — informational, tracks which mode this cook cycle used

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MinerComponent,
                                   collectionRadius, extractionRate, outputInterval, outputTimer,
                                   fuelBuffer, loadedFuelType, fuelRemaining,
                                   outputItem, outputBuffer, outputBufferCapacity,
                                   powerUsage, runningOnPower)
};