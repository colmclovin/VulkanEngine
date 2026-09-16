#include "FuelDatabase.h"

std::unordered_map<ItemId, FuelDef> FuelDatabase::s_Fuels;

void FuelDatabase::Init() {
    s_Fuels[ItemId::Wood] = { 5.0f };
    s_Fuels[ItemId::Coal] = { 12.0f };   // burns longer than wood — tune to taste
}

const FuelDef* FuelDatabase::TryGet(ItemId item)
{
    auto it = s_Fuels.find(item);
    return it != s_Fuels.end() ? &it->second : nullptr;

}