#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera2D {
public:
	Camera2D(float width, float height);

	// Camera controls
	void SetPosition(const glm::vec2& position);
	void Move(const glm::vec2& delta);
	void SetZoom(float zoom);
	void Zoom(float delta);

	// Getters
	const glm::vec2& GetPosition() const { return m_Position; }
	float GetZoom() const { return m_Zoom; }
	glm::mat4 GetViewMatrix() const;
	glm::mat4 GetProjectionMatrix() const;
	glm::mat4 GetViewProjectionMatrix() const;

	// Screen to world conversion
	glm::vec2 ScreenToWorld(const glm::vec2& screenPos) const;

	// Update aspect ratio if window resizes
	void SetViewportSize(float width, float height);

private:
	void RecalculateMatrices();

	glm::vec2 m_Position;
	float m_Zoom;
	float m_AspectRatio;
	float m_ViewportWidth;
	float m_ViewportHeight;

	glm::mat4 m_ViewMatrix;
	glm::mat4 m_ProjectionMatrix;
	glm::mat4 m_ViewProjectionMatrix;

	bool m_RecalculateView;
	bool m_RecalculateProjection;
};
