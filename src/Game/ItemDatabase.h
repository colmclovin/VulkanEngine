#pragma once

#include <string>
#include <unordered_map>
#include <memory>
class Mesh;


enum class ItemId {
	None,
	Wood,
	CopperOre,
	CopperPlate,
	IronOre,
	IronPlate,
    Miner

};
struct ItemDef {
	std::string name;
	int maxStackSize = 100;
	std::string worldMeshPath;
};

class ItemDatabase
{
public:
	static void Init();
	static const ItemDef& Get(ItemId id);
	static std::shared_ptr<Mesh> GetWorldMesh(ItemId id);  
private:
	static std::unordered_map<ItemId, ItemDef> s_Items;
	static std::unordered_map<ItemId, std::shared_ptr<Mesh>> s_MeshCache;
};