// FurnaceRecipeDatabase.cpp
#include "FurnaceRecipeDatabase.h"

std::unordered_map<ItemId, FurnaceRecipe> FurnaceRecipeDatabase::s_Recipes;

void FurnaceRecipeDatabase::Init() {
    s_Recipes[ItemId::CopperOre] = { ItemId::CopperPlate, 3.0f };
    s_Recipes[ItemId::IronOre] = { ItemId::IronPlate, 3.0f };
}

const FurnaceRecipe *FurnaceRecipeDatabase::TryGet(ItemId input) {
    auto it = s_Recipes.find(input);
    return it != s_Recipes.end() ? &it->second : nullptr;
}