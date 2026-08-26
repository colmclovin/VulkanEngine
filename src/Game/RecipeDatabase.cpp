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
    // add more recipes here
}

const std::vector<Recipe>& RecipeDatabase::GetAll() {
    return s_Recipes;
}