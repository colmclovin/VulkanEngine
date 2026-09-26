// RecipeDatabase.cpp
#include "RecipeDatabase.h"

std::vector<Recipe> RecipeDatabase::s_Recipes;

void RecipeDatabase::Init() {
    s_Recipes.push_back({
        "Copper Plate",
        { { ItemId::CopperOre, 1 } },
        { { ItemId::CopperPlate, 1 } },  
        0.5f
        });
    s_Recipes.push_back({
        "Iron Plate",
        { { ItemId::IronOre, 1 } },
        { { ItemId::IronPlate, 1 } },  
        0.5f
        });
    s_Recipes.push_back({ "Steel Plate",
                          { { ItemId::Steel, 5 } },
                          { { ItemId::SteelPlate, 1 } },
                          0.5f });
    s_Recipes.push_back({ "Steel Frame",
                          { { ItemId::SteelPlate, 5 }, { ItemId::Steel, 5 } },
                          { { ItemId::SteelPlate, 1 } },
                          0.5f });
    s_Recipes.push_back({ "Copper Wire",
                          { { ItemId::CopperPlate, 3 } },
                          { { ItemId::CopperWire, 5 } },
                          0.5f });
    s_Recipes.push_back({ "Pipe",
                          { { ItemId::Steel, 5 } },
                          { { ItemId::Pipe, 5 } },
                          0.5f });
    s_Recipes.push_back({
        "Miner ",
        { { ItemId::IronPlate, 1 }, { ItemId::CopperPlate, 1 } },
        { { ItemId::Miner, 1 } },  
        2.0f
        });
    s_Recipes.push_back({
        "Furnace ",
        { { ItemId::StoneBricks, 10 }, { ItemId::IronPlate, 5 } },
        { { ItemId::Furnace, 1 } },  
        1.0f
        });
    s_Recipes.push_back({
        "Assembler ",
        { { ItemId::IronPlate, 1 }, { ItemId::CopperPlate, 1 } },
        { { ItemId::Assembler, 1 } },  
        3.0f
        });
    s_Recipes.push_back({
        "Belt ",
        { { ItemId::IronPlate, 1 }, { ItemId::CopperPlate, 1 } },
        { { ItemId::Belt, 1 } },  
        1.0f
        });
    s_Recipes.push_back({
        "Inserter ",
        { { ItemId::IronPlate, 1 }, { ItemId::CopperPlate, 1 } },
        { { ItemId::Inserter, 1 } },  
        1.0f
        });
    s_Recipes.push_back({ "Tech Point",
                          { { ItemId::IronPlate, 5 }, { ItemId::CopperPlate, 5 } },
                          { { ItemId::TechPoint, 1 } },
                          5.0f });
    s_Recipes.push_back({ "Power Pole ",
                          { { ItemId::Wood, 5 }, { ItemId::CopperPlate, 5 } },
                          { { ItemId::PowerPole, 1 } },
                          0.5f });
    s_Recipes.push_back({ "Coal Generator ",
                          { { ItemId::IronPlate, 1 }, { ItemId::CopperPlate, 1 } },
                          { { ItemId::CoalGenerator, 1 } },
                          1.0f });
}

const std::vector<Recipe>& RecipeDatabase::GetAll() {
    return s_Recipes;
}