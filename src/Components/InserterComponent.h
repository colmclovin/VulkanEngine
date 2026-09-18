#pragma once
#include "../Game/ItemDatabase.h"
#include <glm/glm.hpp>
#include "../Helpers/GlmSerialization.h"



struct InserterComponent {
    glm::vec3 facing = glm::vec3(1, 0, 0); // set at placement time
    float swingTime = 0.5f; // seconds to move an item from source to target
    float timer = 0.0f;
    bool holdingItem = false;
    ItemId heldItem = ItemId::None;

    float powerUsage = 1.0f; // NEW — power units consumed per second while running on power
    bool runningOnPower = false; // NEW — informational, tracks which mode this cook cycle used
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(InserterComponent, facing, swingTime, timer, holdingItem, heldItem, powerUsage, runningOnPower)
};