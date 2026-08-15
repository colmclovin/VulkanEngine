// HarvestableComponent.h
#pragma once
#include "../Game/ItemDatabase.h"

struct HarvestableComponent {
    float health = 100.0f;
    float maxHealth = 100.0f;
    ItemId yieldItem = ItemId::Wood;
    int yieldPerHit = 1;
    int yieldOnDestroy = 5;
};