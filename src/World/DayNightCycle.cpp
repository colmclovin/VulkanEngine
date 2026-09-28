// DayNightCycle.cpp
#include "DayNightCycle.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

void DayNightCycle::Update(float deltaTime) {
	m_TimeOfDay += deltaTime / dayLengthSeconds;
	if (m_TimeOfDay >= 1.0f) m_TimeOfDay -= 1.0f;
}

glm::vec3 DayNightCycle::GetSunDirection() const {
	float angle = m_TimeOfDay * glm::two_pi<float>() - glm::half_pi<float>();
    // Sun arcs across the sky rising in one direction, setting in the opposite —
    // both a horizontal axis (X) and height (Y) vary together, producing a true overhead arc.
    float height = sin(angle);
    float horizontal = cos(angle);

    return glm::normalize(glm::vec3(horizontal, height, 0.0f)); // sweeps across X, not fixed at 0
}


glm::vec3 DayNightCycle::GetSunColor() const {
	float height = GetSunDirection().y; // -1 (below horizon) to 1 (overhead)

	glm::vec3 dayColor(1.0f, 0.95f, 0.85f);
	glm::vec3 sunsetColor(1.0f, 0.5f, 0.3f);
	glm::vec3 nightColor(0.15f, 0.2f, 0.4f);

	if (height > 0.2f) {
		// Full daylight, with a gentle blend toward sunset as we approach the threshold
		float t = glm::clamp((height - 0.2f) / 0.3f, 0.0f, 1.0f); // ramps up over a wider band
		return glm::mix(sunsetColor, dayColor, t);
	} else if (height > -0.2f) {
		// Sunrise/sunset band, blending toward night
		float t = glm::clamp((height + 0.2f) / 0.4f, 0.0f, 1.0f);
		return glm::mix(nightColor, sunsetColor, t);
	} else {
		// Deep night — could still blend further toward nightColor if you want more gradation
		float t = glm::clamp((height + 0.4f) / 0.2f, 0.0f, 1.0f);
		return glm::mix(nightColor * 0.7f, nightColor, t); // slightly darker at true midnight
	}
}

float DayNightCycle::GetAmbientIntensity() const {
	float height = GetSunDirection().y;
	return glm::clamp(0.15f + height * 0.35f, 0.05f, 0.2f);   // dimmer ambient at night, brighter at noon
}

glm::mat4 DayNightCycle::GetLightSpaceMatrix(glm::vec3 focusPoint, float orthoSize, float nearPlane, float farPlane) const {
	glm::vec3 sunDir = GetSunDirection();

	// Position a virtual "light camera" far along the sun direction, looking back at the focus point
	glm::vec3 lightPos = focusPoint + sunDir * (farPlane * 0.5f);

	glm::mat4 lightView = glm::lookAt(lightPos, focusPoint, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 lightProj = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);
	lightProj[1][1] *= -1.0f;   // same Vulkan Y-flip your camera projection already applies

	return lightProj * lightView;
}
