// DayNightCycle.cpp
#include "DayNightCycle.h"
#include <glm/gtc/constants.hpp>
#include <cmath>

void DayNightCycle::Update(float deltaTime) {
    m_TimeOfDay += deltaTime / dayLengthSeconds;
    if (m_TimeOfDay >= 1.0f) m_TimeOfDay -= 1.0f;
}

glm::vec3 DayNightCycle::GetSunDirection() const {
    float angle = m_TimeOfDay * glm::two_pi<float>() - glm::half_pi<float>();
    return glm::normalize(glm::vec3(0.0f, sin(angle), cos(angle)));   // pure arc in the Y-Z plane
}

glm::vec3 DayNightCycle::GetSunColor() const {
    float height = GetSunDirection().y;   // -1 (below horizon) to 1 (overhead)

    if (height > 0.3f) {
        return glm::vec3(1.0f, 0.95f, 0.85f);   // daylight, warm white
    }
    else if (height > -0.1f) {
        // sunrise/sunset transition — blend toward orange
        float t = (height + 0.1f) / 0.4f;
        return glm::mix(glm::vec3(1.0f, 0.5f, 0.3f), glm::vec3(1.0f, 0.95f, 0.85f), t);
    }
    else {
        return glm::vec3(0.15f, 0.2f, 0.4f);   // night, cool dim blue
    }
}

float DayNightCycle::GetAmbientIntensity() const {
    float height = GetSunDirection().y;
    return glm::clamp(0.15f + height * 0.35f, 0.05f, 0.5f);   // dimmer ambient at night, brighter at noon
}