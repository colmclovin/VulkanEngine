// Recipe.h
#pragma once
#include "ItemDatabase.h"
#include <vector>

struct RecipeIngredient {
    ItemId item;
    int count;
};

struct Recipe {
    std::string name;
    std::vector<RecipeIngredient> inputs;
    std::vector<RecipeIngredient> outputs;
    float craftTime = 0.0f;   // seconds; instant crafting can just use 0 for now
};