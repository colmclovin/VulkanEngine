// OreDatabase.cpp
#include "OreDatabase.h"

std::vector<OreDef> OreDatabase::s_Ores;

void OreDatabase::Init() {
    s_Ores.push_back({ ItemId::CopperOre, 0.35f, 0.0f, 200.0f, 0.05f });
    s_Ores.push_back({ ItemId::IronOre, 0.30f, 0.0f, 200.0f, 0.05f });
    s_Ores.push_back({ ItemId::Coal, 0.25f, 0.0f, 150.0f, 0.04f });
    s_Ores.push_back({ ItemId::StoneOre, 0.25f, 0.0f, 150.0f, 0.04f });
    s_Ores.push_back({ ItemId::UraniumOre, 0.05f, 800.0f, 400.0f, 0.2f }); // rare, far-only, scales fast once unlocked by distance
}

const std::vector<OreDef> &OreDatabase::GetAll() {
    return s_Ores;
}
glm::vec3 OreDatabase::GetOreColor(ItemId item) {
    switch (item) {
    case ItemId::CopperOre: return glm::vec3(0.85f, 0.45f, 0.25f);
    case ItemId::IronOre: return glm::vec3(0.55f, 0.55f, 0.65f);
    case ItemId::Coal: return glm::vec3(0.15f, 0.15f, 0.15f);
    case ItemId::StoneOre: return glm::vec3(0.5f, 0.5f, 0.5f);
    case ItemId::UraniumOre: return glm::vec3(0.4f, 0.9f, 0.3f); // distinct, eye-catching — it's rare, should stand out
    default: return glm::vec3(1.0f);
    }
}