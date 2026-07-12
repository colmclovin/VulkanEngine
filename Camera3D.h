#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera3D {
public:
	Camera3D(float fov, float aspectRatio, float nearPlane, float farPlane);

	// Camera controls
	void SetPosition(const glm::vec3& position);
	void Move(const glm::vec3& delta);
	void SetTarget(const glm::vec3& target);
	void SetIsometricAngle(float pitch, float yaw);
	void SetDistance(float distance);
	void MoveDistance(float delta);

	// Orbit controls
	void OrbitAroundTarget(float deltaYaw, float deltaPitch);

	// Getters
	const glm::vec3& GetPosition() const { return m_Position; }
	const glm::vec3& GetTarget() const { return m_Target; }
	float GetDistance() const { return m_Distance; }
	glm::mat4 GetViewMatrix() const;
	glm::mat4 GetProjectionMatrix() const;
	glm::mat4 GetViewProjectionMatrix() const;

	// Screen to world conversion (raycast)
	glm::vec3 ScreenToWorldRay(const glm::vec2& screenPos, float viewportWidth, float viewportHeight) const;

	// Update aspect ratio if window resizes
	void SetAspectRatio(float aspectRatio);
	void SetFOV(float fov);

private:
	void RecalculateMatrices();
	void UpdatePositionFromTarget();

	// Camera properties
	glm::vec3 m_Position;
	glm::vec3 m_Target;
	glm::vec3 m_Up;

	float m_Pitch; // Angle from horizontal (in radians)
	float m_Yaw;   // Rotation around vertical axis (in radians)
	float m_Distance; // Distance from target

	// Projection properties
	float m_FOV;
	float m_AspectRatio;
	float m_NearPlane;
	float m_FarPlane;

	// Matrices
	glm::mat4 m_ViewMatrix;
	glm::mat4 m_ProjectionMatrix;
	glm::mat4 m_ViewProjectionMatrix;

	bool m_RecalculateView;
	bool m_RecalculateProjection;
};
