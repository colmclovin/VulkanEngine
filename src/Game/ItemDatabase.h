#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <JSON/json.hpp>
#include "../Engine/VulkanEngine.h"
#include "../Renderer/MeshRenderer.h"
class Mesh;
class VulkanEngine;

enum class ItemId {
	None,
	Wood,
	CopperOre,
	CopperPlate,
	IronOre,
	IronPlate,
	Coal,
    Miner,
	Furnace,
	Assembler,
	Belt,
	Inserter,
    TechPoint,
	PowerPole,
	CoalGenerator
};
struct ItemDef {
	std::string name;
	int maxStackSize = 100;
	std::string worldMeshPath;
};

void to_json(nlohmann::json &j, const ItemId &id);
void from_json(const nlohmann::json &j, ItemId &id);


class ItemDatabase
{
public:
	static void Init();
	static const ItemDef& Get(ItemId id);
    static std::shared_ptr<Mesh> GetWorldMesh(ItemId id, VulkanEngine *engine, MeshRenderer *meshRenderer);  

	static std::string ToString(ItemId id);
    static ItemId FromString(const std::string &name);

private:
	static std::unordered_map<ItemId, ItemDef> s_Items;
	static std::unordered_map<ItemId, std::shared_ptr<Mesh>> s_MeshCache;
};