#pragma once
#include "../Game/ItemDatabase.h"

struct MinerComponent {
    float collectionRadius = 3.0f; // how far around the miner it reaches into the ResourceMap
    float extractionRate = 1.0f; // resource units extracted per second, while fueled
    float outputInterval = 1.0f; // seconds between producing one item into its output buffer
    float outputTimer = 0.0f; // internal countdown

    ItemId fuelItem = ItemId::Wood;
    int fuelBuffer = 0; // how many fuel items currently loaded
    float fuelBurnTime = 5.0f; // seconds of runtime per single fuel item
    float fuelRemaining = 0.0f; // seconds left on the currently-burning fuel item

    ItemId outputItem = ItemId::None; // set once, based on what resource is under it
    int outputBuffer = 0; // items waiting to be collected/transported
    int outputBufferCapacity = 50;
};