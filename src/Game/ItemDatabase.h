#pragma once

#include <string>
#include <unordered_map>


enum class ItemId {
	None,
	Wood,
	CopperOre,
	IronOre

};
struct ItemDef {
	std::string name;
	int maxStackSize = 100;
};

class ItemDatabase
{
public:
	static void Init();
	static const ItemDef& Get(ItemId id);

private:
	static std::unordered_map<ItemId, ItemDef> s_Items;

};