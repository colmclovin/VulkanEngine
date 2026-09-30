// OreDepositMap.h
#pragma once
#include <optional>
#include <glm/glm.hpp>
#include "ItemDatabase.h"

enum class OreType {
    None = -1,
    Copper = 0,
    Iron = 1,
    Coal = 2,
    Uranium = 3
};

struct OreDepositInfo {
    ItemId item;
    float amount;
};

class OreDepositMap {
public:
    static std::optional<OreDepositInfo> GetDepositAt(float worldX, float worldZ, int seed);

private:
    static float Fract(float x);
    static float HashToFloat(float cellX, float cellZ, float seed, float saltX, float saltZ);
};