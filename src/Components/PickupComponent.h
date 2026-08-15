// PickupComponent.h
#pragma once
#include "../Game/ItemDatabase.h"

struct PickupComponent {
    ItemId item = ItemId::None;
    int count = 1;
};