// AssemblerComponent.h
#pragma once

struct AssemblerComponent {
    int selectedRecipeIndex = -1; // index into RecipeDatabase::GetAll()
    float craftTimer = 0.0f;
    bool isCrafting = false;

        float powerUsage = 5.0f; // NEW — power units consumed per second while running on power
    bool runningOnPower = false; // NEW — informational, tracks which mode this cook cycle used
};