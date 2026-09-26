#pragma once
#include <glm/glm.hpp>

constexpr int MAX_POINT_LIGHTS = 16;

struct PointLight {
    glm::vec4 position;   // xyz used, w unused (padding)
    glm::vec4 color;      // rgb = color, w = intensity
};

struct LightingUBO {
    glm::mat4 viewProj;         // NEW — camera's combined view*proj, moved here from being embedded in push constants
    glm::vec4 sunDirection;
    glm::vec4 sunColor;
    glm::mat4 lightSpaceMatrix;
    int numPointLights = 0;
    float _padding[3];
    PointLight pointLights[MAX_POINT_LIGHTS];
};