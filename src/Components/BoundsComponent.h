// BoundsComponent.h
#pragma once
#include <glm/glm.hpp>

struct BoundsComponent {
    glm::vec3 halfExtents = glm::vec3(0.5f); // local-space half-size (AABB, before world transform)
};