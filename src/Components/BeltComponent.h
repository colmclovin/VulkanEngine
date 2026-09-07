#pragma once
#include "../Game/ItemDatabase.h"
#include <glm/glm.hpp>

struct BeltComponent {
    glm::vec3 direction = glm::vec3(1, 0, 0); // facing, set at placement time
    ItemId heldItem = ItemId::None;
    float progress = 0.0f; // 0 = just entered, 1 = ready to hand off
    float speed = 1.0f; // progress units per second
};