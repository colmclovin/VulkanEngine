// OreDatabase.h
#pragma once
#include "ItemDatabase.h" // or wherever ItemId lives
#include <unordered_map>
#include <vector>

struct OreDef {
    ItemId item;
    float rarity; // 0-1, lower = rarer. Used as a weight in deposit-type selection.
    float minDistanceFromOrigin; // e.g. uranium requires being far from spawn
    float baseAmount; // per-cell ore amount near origin
    float amountPerDistance; // additional amount per world-unit of distance from origin
};

class OreDatabase {
public:
    static void Init();
    static const std::vector<OreDef> &GetAll();
    static glm::vec3 GetOreColor(ItemId item);

private:
    static std::vector<OreDef> s_Ores;
};