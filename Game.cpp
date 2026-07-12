#include "Game.h"
#include "VulkanEngine.h"
#include <GLFW/glfw3.h>
#include <iostream>

Game::Game(VulkanEngine* engine, GLFWwindow* window)
	: m_VulkanEngine(engine)
	, m_Window(window)
	, m_MousePos(0.0f, 0.0f)
	, m_IsPanning(false)
	, m_SelectedBuilding(BuildingType::Conveyor)
	, m_IsPlacingBuilding(false)
{
}

Game::~Game() {
}

void Game::Init() {
	// Initialize renderer
	Renderer2D::Init(m_VulkanEngine);

	// Get window dimensions for aspect ratio
	int windowWidth, windowHeight;
	glfwGetWindowSize(m_Window, &windowWidth, &windowHeight);
	float aspectRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);

	// Create 2D camera for UI/legacy rendering
	m_Camera2D = std::make_unique<Camera2D>(static_cast<float>(windowWidth), static_cast<float>(windowHeight));

	// Create 3D camera for isometric view
	m_Camera3D = std::make_unique<Camera3D>(
		glm::radians(45.0f),  // FOV
		aspectRatio,           // Aspect ratio
		0.1f,                  // Near plane
		1000.0f                // Far plane
	);

	// Create game world (50x50 grid)
	m_World = std::make_unique<GameWorld>(50, 50);

	// Position 3D camera for isometric view centered on the grid
	float tileSize = m_World->GetTileSize();
	float worldCenterX = (m_World->GetGridWidth() / 2.0f) * tileSize;
	float worldCenterY = (m_World->GetGridHeight() / 2.0f) * tileSize;

	// Set camera to look at world center from an isometric angle
	m_Camera3D->SetTarget(glm::vec3(worldCenterX, 0.0f, worldCenterY));
	m_Camera3D->SetIsometricAngle(glm::radians(45.0f), glm::radians(45.0f));
	m_Camera3D->SetDistance(80.0f);

	// Also set 2D camera for now (during transition)
	m_Camera2D->SetPosition(glm::vec2(worldCenterX, worldCenterY));
	m_Camera2D->SetZoom(0.5f);

	// Create resource nodes scattered around the map
	m_World->CreateResourceNode(ItemType::IronOre, 5, 5, 1000);
	m_World->CreateResourceNode(ItemType::CopperOre, 10, 8, 800);
	m_World->CreateResourceNode(ItemType::IronOre, 25, 25, 1500);
	m_World->CreateResourceNode(ItemType::CopperOre, 35, 15, 900);
	m_World->CreateResourceNode(ItemType::IronOre, 15, 35, 1200);

	// Create a production line near (5,5)
	m_World->CreateBuilding(BuildingType::Miner, 5, 6);
	m_World->CreateBuilding(BuildingType::Conveyor, 5, 7);
	m_World->CreateBuilding(BuildingType::Conveyor, 5, 8);
	m_World->CreateBuilding(BuildingType::Conveyor, 6, 8);
	m_World->CreateBuilding(BuildingType::Furnace, 7, 8);

	// Create another production area near center
	m_World->CreateBuilding(BuildingType::Miner, 25, 26);
	m_World->CreateBuilding(BuildingType::Conveyor, 26, 26);
	m_World->CreateBuilding(BuildingType::Furnace, 27, 26);
	m_World->CreateBuilding(BuildingType::Chest, 28, 26);

	std::cout << "Game initialized successfully!" << std::endl;
	std::cout << "=== Controls ===" << std::endl;
	std::cout << "WASD - Move camera" << std::endl;
	std::cout << "Mouse Wheel - Zoom in/out" << std::endl;
	std::cout << "Right Mouse - Pan camera" << std::endl;
	std::cout << "1/2/3 - Select building to place" << std::endl;
	std::cout << "Left Click - Place selected building" << std::endl;
	std::cout << "================" << std::endl;
}

void Game::Update(float deltaTime) {
	HandleInput(deltaTime);

	// Update game systems
	m_World->UpdateConveyors(deltaTime);
	m_World->UpdateProduction(deltaTime);
	m_World->UpdateMining(deltaTime);
}

void Game::Render() {
	Renderer2D::BeginScene(*m_Camera);

	RenderGrid();
	RenderWorld();
	RenderUI();

	Renderer2D::EndScene();
}

void Game::Shutdown() {
	m_World.reset();
	m_Camera.reset();
	Renderer2D::Shutdown();
}

void Game::HandleInput(float deltaTime) {
	float cameraSpeed = 300.0f * deltaTime;

	// Camera movement (WASD)
	if (glfwGetKey(m_Window, GLFW_KEY_W) == GLFW_PRESS) {
		m_Camera->Move(glm::vec2(0.0f, -cameraSpeed));
	}
	if (glfwGetKey(m_Window, GLFW_KEY_S) == GLFW_PRESS) {
		m_Camera->Move(glm::vec2(0.0f, cameraSpeed));
	}
	if (glfwGetKey(m_Window, GLFW_KEY_A) == GLFW_PRESS) {
		m_Camera->Move(glm::vec2(-cameraSpeed, 0.0f));
	}
	if (glfwGetKey(m_Window, GLFW_KEY_D) == GLFW_PRESS) {
		m_Camera->Move(glm::vec2(cameraSpeed, 0.0f));
	}

	// Building selection (number keys)
	if (glfwGetKey(m_Window, GLFW_KEY_1) == GLFW_PRESS) {
		m_SelectedBuilding = BuildingType::Conveyor;
		m_IsPlacingBuilding = true;
	}
	if (glfwGetKey(m_Window, GLFW_KEY_2) == GLFW_PRESS) {
		m_SelectedBuilding = BuildingType::Furnace;
		m_IsPlacingBuilding = true;
	}
	if (glfwGetKey(m_Window, GLFW_KEY_3) == GLFW_PRESS) {
		m_SelectedBuilding = BuildingType::Assembler;
		m_IsPlacingBuilding = true;
	}
}

void Game::OnMouseButton(int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		// Place building
		if (m_IsPlacingBuilding) {
			glm::vec2 worldPos = m_Camera->ScreenToWorld(m_MousePos);
			glm::ivec2 gridPos = m_World->WorldToGrid(worldPos);

			auto entity = m_World->CreateBuilding(m_SelectedBuilding, gridPos.x, gridPos.y);
			if (entity != entt::null) {
				std::cout << "Placed building at (" << gridPos.x << ", " << gridPos.y << ")" << std::endl;
			}
		}
	}

	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		m_IsPanning = true;
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
		m_IsPanning = false;
	}
}

void Game::OnMouseMove(double xpos, double ypos) {
	glm::vec2 newMousePos(xpos, ypos);

	if (m_IsPanning) {
		glm::vec2 delta = newMousePos - m_MousePos;
		m_Camera->Move(glm::vec2(-delta.x, -delta.y) / m_Camera->GetZoom());
	}

	m_MousePos = newMousePos;
}

void Game::OnScroll(double xoffset, double yoffset) {
	m_Camera->Zoom(static_cast<float>(yoffset) * 0.1f);
}

void Game::RenderWorld() {
	auto& registry = m_World->GetRegistry();
	auto view = registry.view<Transform, SpriteComponent>();

	for (auto entity : view) {
		auto& transform = view.get<Transform>(entity);
		auto& sprite = view.get<SpriteComponent>(entity);

		Renderer2D::DrawQuad(
			transform.position,
			transform.scale,
			transform.rotation,
			sprite.color
		);
	}
}

void Game::RenderGrid() {
	// Draw grid lines
	int gridWidth = m_World->GetGridWidth();
	int gridHeight = m_World->GetGridHeight();
	float tileSize = m_World->GetTileSize();

	glm::vec4 gridColor(0.2f, 0.2f, 0.2f, 0.3f);

	// Vertical lines
	for (int x = 0; x <= gridWidth; x++) {
		glm::vec2 start(x * tileSize, 0.0f);
		glm::vec2 end(x * tileSize, gridHeight * tileSize);
		// Draw thin vertical line
		Renderer2D::DrawQuad(
			glm::vec2(x * tileSize, gridHeight * tileSize / 2.0f),
			glm::vec2(1.0f, gridHeight * tileSize),
			gridColor
		);
	}

	// Horizontal lines
	for (int y = 0; y <= gridHeight; y++) {
		Renderer2D::DrawQuad(
			glm::vec2(gridWidth * tileSize / 2.0f, y * tileSize),
			glm::vec2(gridWidth * tileSize, 1.0f),
			gridColor
		);
	}
}

void Game::RenderUI() {
	// TODO: Render UI elements (selected building, stats, etc.)
}
