// AssemblerComponent.h
#pragma once
#include "../Helpers/GlmSerialization.h"


struct AssemblerComponent {
    int selectedRecipeIndex = -1; // index into RecipeDatabase::GetAll()
    float craftTimer = 0.0f;
    bool isCrafting = false;

        float powerUsage = 5.0f; // NEW — power units consumed per second while running on power
    bool runningOnPower = false; // NEW — informational, tracks which mode this cook cycle used

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(AssemblerComponent, selectedRecipeIndex, craftTimer, isCrafting, powerUsage, runningOnPower)
};