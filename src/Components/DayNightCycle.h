#pragma once
#include <glm/glm.hpp>

class DayNightCycle {
public:
    void Update(float deltaTime);

    glm::vec3 GetSunDirection() const;
    glm::vec3 GetSunColor() const;
    float GetAmbientIntensity() const;
    float GetTimeOfDay() const { return m_TimeOfDay; }   // 0.0 - 1.0, 0 = midnight, 0.5 = noon

    float dayLengthSeconds = 15.0f;   // how long a full day takes in real seconds — tune to taste

private:
    float m_TimeOfDay = 0.25f;   // start at sunrise-ish
};