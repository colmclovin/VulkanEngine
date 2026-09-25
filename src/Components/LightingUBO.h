#pragma once
#pragma once
#include <glm/glm.hpp>

struct LightingUBO {
    glm::vec4 sunDirection;   // xyz used, w padding
    glm::vec4 sunColor;       // xyz used, w = ambient intensity, packed together to save a slot
};