// RecipeDatabase.h
#pragma once
#include "Recipe.h"
#include <vector>

class RecipeDatabase {
public:
    static void Init();
    static const std::vector<Recipe>& GetAll();

private:
    static std::vector<Recipe> s_Recipes;
};