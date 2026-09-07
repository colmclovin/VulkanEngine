// AssemblerComponent.h
#pragma once

struct AssemblerComponent {
    int selectedRecipeIndex = -1; // index into RecipeDatabase::GetAll()
    float craftTimer = 0.0f;
    bool isCrafting = false;
};