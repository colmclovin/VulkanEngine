// PlaceableDatabase.h
#pragma once
#include "ItemDatabase.h"
#include <string>
#include <unordered_map>
#include <functional>
#include <glm/glm.hpp>

struct PlaceableDef {
    std::string meshPath;      // model to load for the placed entity
    glm::vec3 scale = glm::vec3(1.0f);
    // extend later: machine behavior tag, footprint size for grid snapping, etc.
};

class PlaceableDatabase {
public:
    static void Init();
    static const PlaceableDef* TryGet(ItemId item);   // nullptr if item isn't placeable

private:
    static std::unordered_map<ItemId, PlaceableDef> s_Placeables;
};