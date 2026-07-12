#pragma once

// Disable narrowing conversion warnings for EnTT
#pragma warning(push)
#pragma warning(disable: 4244 4267 4305)
#include "GameWorld.h"
#pragma warning(pop)

#include "Camera2D.h"
#include "Camera3D.h"
#include "Renderer2D.h"
#include <memory>

// Forward declarations
class VulkanEngine;
struct GLFWwindow;

class Game {
public:
	Game(VulkanEngine* engine, GLFWwindow* window);
	~Game();

	void Init();
	void Update(float deltaTime);
	void Render();
	void Shutdown();

	// Input handling
	void HandleInput(float deltaTime);
	void OnMouseButton(int button, int action, int mods);
	void OnMouseMove(double xpos, double ypos);
	void OnScroll(double xoffset, double yoffset);

private:
	// Core systems
	VulkanEngine* m_VulkanEngine;
	GLFWwindow* m_Window;
	std::unique_ptr<GameWorld> m_World;
	std::unique_ptr<Camera2D> m_Camera2D;
	std::unique_ptr<Camera3D> m_Camera3D;

	// Input state
	glm::vec2 m_MousePos;
	bool m_IsPanning;

	// Game state
	BuildingType m_SelectedBuilding;
	bool m_IsPlacingBuilding;

	// Rendering
	void RenderWorld();
	void RenderGrid();
	void RenderUI();
};
