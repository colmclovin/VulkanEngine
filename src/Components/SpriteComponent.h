#pragma once
#include "TransformComponent.h"
#include <glm/glm.hpp>

struct SpriteComponent {
    TransformComponent transform;
    glm::vec4 color = glm::vec4(1.0f);
    int layer = 0;
};
