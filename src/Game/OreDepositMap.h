// OreDepositMap.h
#pragma once
#include "ItemDatabase.h"
#include "OreDatabase.h"
#include <optional>

struct OreDepositInfo {
    ItemId item;
    float amount; // per-cell amount at this specific point, already distance-scaled
};

class OreDepositMap {
public:
    static std::optional<OreDepositInfo> GetDepositAt(float worldX, float worldZ, int seed);
};