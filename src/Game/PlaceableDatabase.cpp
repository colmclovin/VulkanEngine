// PlaceableDatabase.cpp
#include "PlaceableDatabase.h"

std::unordered_map<ItemId, PlaceableDef> PlaceableDatabase::s_Placeables;

void PlaceableDatabase::Init() {
    s_Placeables[ItemId::Wood] = { "Assets/Models/Wood.glb", glm::vec3(1.0f) };
    // add more as you get building item types, e.g. ItemId::Furnace, ItemId::Belt
}

const PlaceableDef* PlaceableDatabase::TryGet(ItemId item) {
    auto it = s_Placeables.find(item);
    return it != s_Placeables.end() ? &it->second : nullptr;
}