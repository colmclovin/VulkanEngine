#include "UnitDatabase.h"

std::unordered_map<ItemId, UnitDef> UnitDatabase::s_Defs;

void UnitDatabase::Init() {
    UnitDef soldier;
    soldier.item = ItemId::Soldier;
    soldier.maxHealth = 100.0f;
    soldier.stats.damage = 10.0f;
    soldier.stats.damageType = DamageType::Physical;
    soldier.stats.range = 8.0f;
    s_Defs[soldier.item] = soldier;

    // later: SoldierLongRange, SoldierFire, etc.
}

const UnitDef *UnitDatabase::TryGet(ItemId item) {
    auto it = s_Defs.find(item);
    return it != s_Defs.end() ? &it->second : nullptr;
}