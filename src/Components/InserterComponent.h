#pragma once
#include "../Game/ItemDatabase.h"
#include <glm/glm.hpp>

struct InserterComponent {
    glm::vec3 facing = glm::vec3(1, 0, 0); // set at placement time
    float swingTime = 0.5f; // seconds to move an item from source to target
    float timer = 0.0f;
    bool holdingItem = false;
    ItemId heldItem = ItemId::None;
};