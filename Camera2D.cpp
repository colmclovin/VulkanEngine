#include "Camera2D.h"

Camera2D::Camera2D(float width, float height)
	: m_Position(0.0f, 0.0f)
	, m_Zoom(1.0f)
	, m_ViewportWidth(width)
	, m_ViewportHeight(height)
	, m_RecalculateView(true)
	, m_RecalculateProjection(true)
{
	m_AspectRatio = width / height;
	RecalculateMatrices();
}

void Camera2D::SetPosition(const glm::vec2& position) {
	m_Position = position;
	m_RecalculateView = true;
}

void Camera2D::Move(const glm::vec2& delta) {
	m_Position += delta;
	m_RecalculateView = true;
}

void Camera2D::SetZoom(float zoom) {
	m_Zoom = glm::clamp(zoom, 0.1f, 10.0f);
	m_RecalculateProjection = true;
}

void Camera2D::Zoom(float delta) {
	m_Zoom = glm::clamp(m_Zoom + delta, 0.1f, 10.0f);
	m_RecalculateProjection = true;
}

glm::mat4 Camera2D::GetViewMatrix() const {
	// Recalculate if needed before returning
	const_cast<Camera2D*>(this)->RecalculateMatrices();
	return m_ViewMatrix;
}

glm::mat4 Camera2D::GetProjectionMatrix() const {
	const_cast<Camera2D*>(this)->RecalculateMatrices();
	return m_ProjectionMatrix;
}

glm::mat4 Camera2D::GetViewProjectionMatrix() const {
	const_cast<Camera2D*>(this)->RecalculateMatrices();
	return m_ViewProjectionMatrix;
}

glm::vec2 Camera2D::ScreenToWorld(const glm::vec2& screenPos) const {
	// Convert screen coordinates to NDC (-1 to 1)
	glm::vec2 ndc;
	ndc.x = (2.0f * screenPos.x) / m_ViewportWidth - 1.0f;
	ndc.y = (2.0f * screenPos.y) / m_ViewportHeight - 1.0f;

	// Convert NDC to world space
	glm::vec4 worldPos = glm::inverse(m_ViewProjectionMatrix) * glm::vec4(ndc, 0.0f, 1.0f);
	return glm::vec2(worldPos.x, worldPos.y);
}

void Camera2D::SetViewportSize(float width, float height) {
	m_ViewportWidth = width;
	m_ViewportHeight = height;
	m_AspectRatio = width / height;
	m_RecalculateProjection = true;
	RecalculateMatrices();
}

void Camera2D::RecalculateMatrices() {
	if (m_RecalculateView) {
		// Create view matrix (camera transform)
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(m_Position, 0.0f));
		m_ViewMatrix = glm::inverse(transform);
		m_RecalculateView = false;
	}

	if (m_RecalculateProjection) {
		// Orthographic projection for 2D
		float orthoLeft = -m_ViewportWidth / (2.0f * m_Zoom);
		float orthoRight = m_ViewportWidth / (2.0f * m_Zoom);
		float orthoBottom = -m_ViewportHeight / (2.0f * m_Zoom);
		float orthoTop = m_ViewportHeight / (2.0f * m_Zoom);

		m_ProjectionMatrix = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, -1.0f, 1.0f);
		m_RecalculateProjection = false;
	}

	m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}
