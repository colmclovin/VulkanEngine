#include "ItemDatabase.h"
#include "../Components/Mesh.h"
#include "../Components/ModelLoader.h"
#include <stdexcept>

std::unordered_map<ItemId, ItemDef> ItemDatabase::s_Items;
std::unordered_map<ItemId, std::shared_ptr<Mesh>> ItemDatabase::s_MeshCache;   // ADD THIS LINE


void ItemDatabase::Init() {
	s_Items[ItemId::Wood] = { "Wood", 999, "Assets/Models/Wood.glb" };
	s_Items[ItemId::CopperOre] = { "Copper Ore", 999, "Assets/Models/CopperOre.glb" };
    s_Items[ItemId::CopperPlate] = { "Copper Plate", 999, "Assets/Models/CopperOre.glb" };
	s_Items[ItemId::IronOre] = { "Iron Ore", 999, "Assets/Models/IronOre.glb" };
    s_Items[ItemId::IronPlate] = { "Iron Plate", 999, "Assets/Models/IronOre.glb" };
};

const ItemDef& ItemDatabase::Get(ItemId id) {
    auto it = s_Items.find(id);
    if (it == s_Items.end()) {
        throw std::runtime_error("ItemDatabase::Get called with unregistered ItemId: " + std::to_string(static_cast<int>(id)));
    }
    return it->second;
}

std::shared_ptr<Mesh> ItemDatabase::GetWorldMesh(ItemId id) {
    auto cached = s_MeshCache.find(id);
    if (cached != s_MeshCache.end()) {
        return cached->second;   // already loaded, return the shared instance
    }

    const ItemDef& def = Get(id);
    if (def.worldMeshPath.empty()) {
        return nullptr;   // this item has no world representation (e.g. intangible/currency items later)
    }

    auto mesh = std::make_shared<Mesh>(ModelLoader::LoadModel(def.worldMeshPath));
    s_MeshCache[id] = mesh;
    return mesh;
}