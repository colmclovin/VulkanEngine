#pragma once
#include "../Game/ItemDatabase.h"
#include <glm/glm.hpp>

struct BeltItem {
    ItemId item = ItemId::None;
    float progress = 0.0f; // 0 = just entered this lane, 1 = ready to exit
};

struct BeltLane {
    std::vector<BeltItem> queue; // front() = closest to exit
    int capacity = 2; // 2 tiles of length per lane
};

struct BeltComponent {
    glm::vec3 direction = glm::vec3(1, 0, 0);
    float speed = 1.0f;
    BeltLane leftLane;
    BeltLane rightLane;
};