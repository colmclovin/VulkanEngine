#pragma once
#include "../Components/CombatStatsComponent.h"
#include "../Components/ResistanceComponent.h"
#include "ItemDatabase.h" // wherever ItemId is defined
#include <unordered_map>

struct UnitDef {
    ItemId item = ItemId::None;
    float maxHealth = 100.0f;
    float visionRadius = 40.0f;
    CombatStatsComponent stats;
    ResistanceComponent resist;
};

class UnitDatabase {
public:
    static void Init();
    static const UnitDef *TryGet(ItemId item); // nullptr for non-unit items

private:
    static std::unordered_map<ItemId, UnitDef> s_Defs;
};