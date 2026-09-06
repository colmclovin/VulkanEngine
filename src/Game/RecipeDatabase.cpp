// RecipeDatabase.cpp
#include "RecipeDatabase.h"

std::vector<Recipe> RecipeDatabase::s_Recipes;

void RecipeDatabase::Init() {
    s_Recipes.push_back({
        "Copper Plate",
        { { ItemId::CopperOre, 1 } },
        { { ItemId::CopperPlate, 1 } },  
        0.0f
        });
    s_Recipes.push_back({
        "Iron Plate",
        { { ItemId::IronOre, 1 } },
        { { ItemId::IronPlate, 1 } },  
        0.0f
        });
    s_Recipes.push_back({
        "Miner ",
        { { ItemId::IronPlate, 1 }, { ItemId::CopperPlate, 1 } },
        { { ItemId::Miner, 1 } },  
        0.0f
        });
}

const std::vector<Recipe>& RecipeDatabase::GetAll() {
    return s_Recipes;
}