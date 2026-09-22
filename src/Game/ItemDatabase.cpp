#include "ItemDatabase.h"
#include "../Components/Mesh.h"
#include "../Components/ModelLoader.h"
#include <stdexcept>
#include <JSON/json.hpp>
std::unordered_map<ItemId, ItemDef> ItemDatabase::s_Items;
std::unordered_map<ItemId, std::shared_ptr<Mesh>> ItemDatabase::s_MeshCache;


void ItemDatabase::Init() {
	s_Items[ItemId::Wood] = { "Wood", 999, "Assets/Models/Wood.glb" };
    s_Items[ItemId::StoneOre] = { "Stone Ore", 999, "Assets/Models/StoneOre.glb" };
    s_Items[ItemId::Stone] = { "Stone", 999, "Assets/Models/Stone.glb" };
	s_Items[ItemId::CopperOre] = { "Copper Ore", 999, "Assets/Models/CopperOre.glb" };
    s_Items[ItemId::CopperPlate] = { "Copper Plate", 999, "Assets/Models/CopperPlate.glb" };
    s_Items[ItemId::UraniumOre] = { "Uranium Ore", 999, "Assets/Models/UraniumOre.glb" };
	s_Items[ItemId::IronOre] = { "Iron Ore", 999, "Assets/Models/IronOre.glb" };
    s_Items[ItemId::IronPlate] = { "Iron Plate", 999, "Assets/Models/IronPlate.glb" };
    s_Items[ItemId::Coal] = { "Coal", 999, "Assets/Models/Coal.glb" };
    s_Items[ItemId::Miner] = { "Miner", 999, "Assets/Models/Miner.glb" };
    s_Items[ItemId::Furnace] = { "Furnace", 999, "Assets/Models/Furnace.glb" };
    s_Items[ItemId::Assembler] = { "Assembler", 999, "Assets/Models/Assembler.glb" };
    s_Items[ItemId::Belt] = { "Belt", 999, "Assets/Models/Belt.glb" };
    s_Items[ItemId::Inserter] = { "Inserter", 999, "Assets/Models/Inserter.glb" };
    s_Items[ItemId::TechPoint] = { "Tech Point", 999, "Assets/Models/TechPoint.glb" };
    s_Items[ItemId::PowerPole] = { "Power Pole", 999, "Assets/Models/PowerPole.glb" };
    s_Items[ItemId::CoalGenerator] = { "Coal Generator", 999, "Assets/Models/CoalGenerator.glb" };
    s_Items[ItemId::CopperWire] = { "Copper Wire", 999, "Assets/Models/CopperWire.glb" };
    s_Items[ItemId::IntegratedCircuit] = { "Integrated Circuit", 999, "Assets/Models/IntegratedCircuit.glb" };
    s_Items[ItemId::TechFrame] = { "Tech Frame", 999, "Assets/Models/TechFrame.glb" };
    s_Items[ItemId::Engine] = { "Engine", 999, "Assets/Models/Engine.glb" };
    s_Items[ItemId::Steel] = { "Steel", 999, "Assets/Models/Steel.glb" };
    s_Items[ItemId::SteelPlate] = { "Steel Plate", 999, "Assets/Models/SteelPlate.glb" };
    s_Items[ItemId::SteelFrame] = { "Steel Frame", 999, "Assets/Models/SteelFrame.glb" };
    s_Items[ItemId::Pipe] = { "Pipe", 999, "Assets/Models/Pipe.glb" };
    s_Items[ItemId::Ammunition] = { "Ammunition", 999, "Assets/Models/Ammunition.glb" };
    s_Items[ItemId::RobotFrame] = { "Robot Frame", 999, "Assets/Models/RobotFrame.glb" };
    s_Items[ItemId::Hunter] = { "Hunter", 999, "Assets/Models/Hunter.glb" };
    s_Items[ItemId::Pistol] = { "Pistol", 999, "Assets/Models/Pistol.glb" };
    s_Items[ItemId::SMG] = { "SMG", 999, "Assets/Models/SMG.glb" };
    s_Items[ItemId::LMG] = { "LMG", 999, "Assets/Models/LMG.glb" };
    s_Items[ItemId::Sniper] = { "Sniper", 999, "Assets/Models/Sniper.glb" };
    s_Items[ItemId::Shotgun] = { "Shotgun", 999, "Assets/Models/Shotgun.glb" };
    s_Items[ItemId::FlameThrower] = { "Flame Thrower", 999, "Assets/Models/FlameThrower.glb" };
    s_Items[ItemId::Oil] = { "Oil", 999, "Assets/Models/Oil.glb" };
    s_Items[ItemId::Plastic] = { "Plastic", 999, "Assets/Models/Plastic.glb" };

}


const ItemDef& ItemDatabase::Get(ItemId id) {
    auto it = s_Items.find(id);
    if (it == s_Items.end()) {
        throw std::runtime_error("ItemDatabase::Get called with unregistered ItemId: " + std::to_string(static_cast<int>(id)));
    }
    return it->second;
}

std::shared_ptr<Mesh> ItemDatabase::GetWorldMesh(ItemId id, VulkanEngine *engine, MeshRenderer *meshRenderer) {
    auto cached = s_MeshCache.find(id);
    if (cached != s_MeshCache.end()) {
        return cached->second;   // already loaded, return the shared instance
    }

    const ItemDef& def = Get(id);
    if (def.worldMeshPath.empty()) {
        return nullptr;   // this item has no world representation (e.g. intangible/currency items later)
    }

    auto mesh = std::make_shared<Mesh>(ModelLoader::LoadModel(def.worldMeshPath, engine, meshRenderer));
    s_MeshCache[id] = mesh;
    return mesh;
}


void to_json(nlohmann::json &j, const ItemId &id) {
    j = ItemDatabase::ToString(id);
}
void from_json(const nlohmann::json &j, ItemId &id) {
    id = ItemDatabase::FromString(j.get<std::string>());
}

static const std::unordered_map<ItemId, std::string> s_IdToName = {
    { ItemId::None, "None" },
    { ItemId::Wood, "Wood" },
    { ItemId::Coal, "Coal" },
    { ItemId::CopperOre, "CopperOre" },
    { ItemId::CopperPlate, "CopperPlate" },
    { ItemId::IronOre, "IronOre" },
    { ItemId::IronPlate, "IronPlate" },
    { ItemId::Miner, "Miner" },
    { ItemId::Furnace, "Furnace" },
    { ItemId::Assembler, "Assembler" },
    { ItemId::Belt, "Belt" },
    { ItemId::Inserter, "Inserter" },
    { ItemId::TechPoint, "TechPoint" },
    { ItemId::PowerPole, "PowerPole" },
    { ItemId::CoalGenerator, "CoalGenerator" },
};

std::string ItemDatabase::ToString(ItemId id) {
    auto it = s_IdToName.find(id);
    return it != s_IdToName.end() ? it->second : "None";
}

ItemId ItemDatabase::FromString(const std::string &name) {
    for (auto &[id, str] : s_IdToName) {
        if (str == name) return id;
    }
    return ItemId::None;
}