#pragma once
#include <glm/glm.hpp>
#include <JSON/json.hpp>

struct PointLightComponent {
    glm::vec3 color = glm::vec3(1.0f, 0.6f, 0.2f);
    float intensity = 50.0f;
    float range = 50.0f;
    bool active = false;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(PointLightComponent, color, intensity, range, active)
};