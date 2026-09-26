#pragma once
#include <glm/glm.hpp>

struct PointLightComponent {
    glm::vec3 color = glm::vec3(1.0f, 0.6f, 0.2f);   // warm orange, furnace-glow default
    float intensity = 2.0f;
    float range = 5.0f;
    bool active = false;   // FurnaceSystem sets this true while isCooking, false otherwise
};