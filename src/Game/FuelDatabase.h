#pragma once
#include "ItemDatabase.h"
#include <unordered_map>


struct FuelDef {
	float burnTime = 5.0f;
};

class FuelDatabase
{
public:
	static void Init();
	static const FuelDef* TryGet(ItemId item);
	static bool IsFuel(ItemId item) { return TryGet(item) != nullptr; }
private:
	static std::unordered_map<ItemId, FuelDef> s_Fuels;

};