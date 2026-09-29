#pragma once
#include "../Game/ItemDatabase.h"
#include <glm/glm.hpp>
#include "../Helpers/GlmSerialization.h"


struct BeltItem {
    ItemId item = ItemId::None;
    float progress = 0.0f; // 0 = just entered this lane, 1 = ready to exit
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(BeltItem, item, progress)
};

struct BeltLane {
    std::vector<BeltItem> queue;
    int capacity = 2;
    std::vector<entt::entity> itemEntities;   // NEW — parallel array, NOT serialized (rendering-only, rebuilt on load)
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(BeltLane, queue, capacity)   // unchanged — itemEntities excluded from serialization
};

struct BeltComponent {
    glm::vec3 direction = glm::vec3(1, 0, 0);
    float speed = 1.0f;
    BeltLane leftLane;
    BeltLane rightLane;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(BeltComponent, direction, speed, leftLane, rightLane)
};