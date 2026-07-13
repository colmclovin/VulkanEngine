#pragma once
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <memory>

// Forward declarations
class VulkanContext;
class RenderSystem;
class Camera3D;

// ============================================================================
// Game - Main game logic coordinator
// ============================================================================
class GameApp {
public:
	GameApp();
	~GameApp();

	void Run();

private:
	void Init();
	void LoadResources();
	void CreateInitialEntities();
	void HandleInput(float deltaTime);
	void Update(float deltaTime);
	void Render();
	void Shutdown();

	// Core systems
	std::unique_ptr<VulkanContext> m_VulkanContext;
	std::unique_ptr<RenderSystem> m_RenderSystem;
	std::unique_ptr<Camera3D> m_Camera;
	std::unique_ptr<entt::registry> m_Registry;

	// Game state
	bool m_IsRunning = false;
	float m_CameraDistance = 15.0f;
	float m_CameraYaw = 0.0f;
	float m_CameraPitch = -45.0f;
	glm::vec3 m_CameraTarget = glm::vec3(0.0f);

	// Input state
	bool m_MouseDragging = false;
	double m_LastMouseX = 0.0;
	double m_LastMouseY = 0.0;

	// Selected entity for building placement
	entt::entity m_SelectedEntity = entt::null;
};
