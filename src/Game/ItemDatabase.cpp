#include "ItemDatabase.h"

std::unordered_map<ItemId, ItemDef> ItemDatabase::s_Items;



void ItemDatabase::Init() {
	s_Items[ItemId::Wood] = { "Wood", 999 };
	s_Items[ItemId::CopperOre] = { "Copper Ore", 999 };
	s_Items[ItemId::IronOre] = { "Iron Ore", 999 };

};

const ItemDef& ItemDatabase::Get(ItemId id) {
	return s_Items.at(id);
}
