#include "Camera3D.h"
#include <glm/gtc/constants.hpp>

Camera3D::Camera3D(float fov, float aspectRatio, float nearPlane, float farPlane)
	: m_Position(0.0f, 10.0f, 10.0f)
	, m_Target(0.0f, 0.0f, 0.0f)
	, m_Up(0.0f, 1.0f, 0.0f)
	, m_Pitch(glm::radians(45.0f))  // 45 degrees for isometric
	, m_Yaw(glm::radians(45.0f))    // 45 degrees rotation
	, m_Distance(20.0f)
	, m_FOV(fov)
	, m_AspectRatio(aspectRatio)
	, m_NearPlane(nearPlane)
	, m_FarPlane(farPlane)
	, m_RecalculateView(true)
	, m_RecalculateProjection(true)
{
	UpdatePositionFromTarget();
	RecalculateMatrices();
}

void Camera3D::SetPosition(const glm::vec3& position) {
	m_Position = position;
	m_RecalculateView = true;
}

void Camera3D::Move(const glm::vec3& delta) {
	m_Position += delta;
	m_Target += delta;
	m_RecalculateView = true;
}

void Camera3D::SetTarget(const glm::vec3& target) {
	m_Target = target;
	UpdatePositionFromTarget();
	m_RecalculateView = true;
}

void Camera3D::SetIsometricAngle(float pitch, float yaw) {
	m_Pitch = pitch;
	m_Yaw = yaw;
	UpdatePositionFromTarget();
	m_RecalculateView = true;
}

void Camera3D::SetDistance(float distance) {
	m_Distance = glm::clamp(distance, 1.0f, 500.0f);
	UpdatePositionFromTarget();
	m_RecalculateView = true;
}

void Camera3D::MoveDistance(float delta) {
	m_Distance = glm::clamp(m_Distance + delta, 1.0f, 500.0f);
	UpdatePositionFromTarget();
	m_RecalculateView = true;
}

void Camera3D::OrbitAroundTarget(float deltaYaw, float deltaPitch) {
	m_Yaw += deltaYaw;
	m_Pitch = glm::clamp(m_Pitch + deltaPitch, glm::radians(10.0f), glm::radians(89.0f));
	UpdatePositionFromTarget();
	m_RecalculateView = true;
}

void Camera3D::SetAspectRatio(float aspectRatio) {
	m_AspectRatio = aspectRatio;
	m_RecalculateProjection = true;
}

void Camera3D::SetFOV(float fov) {
	m_FOV = fov;
	m_RecalculateProjection = true;
}

glm::mat4 Camera3D::GetViewMatrix() const {
	const_cast<Camera3D*>(this)->RecalculateMatrices();
	return m_ViewMatrix;
}

glm::mat4 Camera3D::GetProjectionMatrix() const {
	const_cast<Camera3D*>(this)->RecalculateMatrices();
	return m_ProjectionMatrix;
}

glm::mat4 Camera3D::GetViewProjectionMatrix() const {
	const_cast<Camera3D*>(this)->RecalculateMatrices();
	return m_ViewProjectionMatrix;
}

glm::vec3 Camera3D::ScreenToWorldRay(const glm::vec2& screenPos, float viewportWidth, float viewportHeight) const {
	// Convert screen coordinates to NDC
	glm::vec2 ndc;
	ndc.x = (2.0f * screenPos.x) / viewportWidth - 1.0f;
	ndc.y = 1.0f - (2.0f * screenPos.y) / viewportHeight;

	// Convert to clip space
	glm::vec4 clipCoords(ndc.x, ndc.y, -1.0f, 1.0f);

	// Convert to view space
	glm::vec4 eyeCoords = glm::inverse(m_ProjectionMatrix) * clipCoords;
	eyeCoords = glm::vec4(eyeCoords.x, eyeCoords.y, -1.0f, 0.0f);

	// Convert to world space
	glm::vec3 worldRay = glm::vec3(glm::inverse(m_ViewMatrix) * eyeCoords);
	return glm::normalize(worldRay);
}

void Camera3D::RecalculateMatrices() {
	if (m_RecalculateView) {
		// Look at the target from the camera position
		m_ViewMatrix = glm::lookAt(m_Position, m_Target, m_Up);
		m_RecalculateView = false;
	}

	if (m_RecalculateProjection) {
		// Perspective projection for 3D
		m_ProjectionMatrix = glm::perspective(m_FOV, m_AspectRatio, m_NearPlane, m_FarPlane);
		m_RecalculateProjection = false;
	}

	m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

void Camera3D::UpdatePositionFromTarget() {
	// Calculate camera position based on pitch, yaw, and distance from target
	float x = m_Distance * cos(m_Pitch) * cos(m_Yaw);
	float y = m_Distance * sin(m_Pitch);
	float z = m_Distance * cos(m_Pitch) * sin(m_Yaw);

	m_Position = m_Target + glm::vec3(x, y, z);
}
