// FurnaceRecipeDatabase.h
#pragma once
#include "ItemDatabase.h"
#include <unordered_map>

struct FurnaceRecipe {
    ItemId output;
    float cookTime;
};

class FurnaceRecipeDatabase {
public:
    static void Init();
    static const FurnaceRecipe *TryGet(ItemId input);

private:
    static std::unordered_map<ItemId, FurnaceRecipe> s_Recipes;
};