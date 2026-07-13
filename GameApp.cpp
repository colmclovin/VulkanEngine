#include "GameApp.h"
#include "VulkanContext.h"
#include "RenderSystem.h"
#include "ResourceManager.h"
#include "Camera3D.h"
#include "Components.h"
#include "Mesh.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/gtc/constants.hpp>

GameApp::GameApp() {
}

GameApp::~GameApp() {
	Shutdown();
}

void GameApp::Run() {
	Init();

	m_IsRunning = true;
	float lastTime = static_cast<float>(glfwGetTime());

	while (m_IsRunning && !m_VulkanContext->ShouldClose()) {
		m_VulkanContext->PollEvents();

		float currentTime = static_cast<float>(glfwGetTime());
		float deltaTime = currentTime - lastTime;
		lastTime = currentTime;

		HandleInput(deltaTime);
		Update(deltaTime);
		Render();
	}

	Shutdown();
}

void GameApp::Init() {
	std::cout << "=== Initializing Game ===" << std::endl;

	// Initialize Vulkan
	m_VulkanContext = std::make_unique<VulkanContext>();
	m_VulkanContext->Init("Factorio-Like 3D", 1280, 720);

	// Initialize rendering systems
	m_RenderSystem = std::make_unique<RenderSystem>(m_VulkanContext.get());
	m_RenderSystem->Init();

	// Initialize camera
	m_Camera = std::make_unique<Camera3D>(45.0f, 1280.0f / 720.0f, 0.1f, 1000.0f);
	m_Camera->SetTarget(m_CameraTarget);
	m_Camera->SetDistance(m_CameraDistance);
	m_Camera->SetIsometricAngle(glm::radians(m_CameraPitch), glm::radians(m_CameraYaw));

	// Create ECS registry
	m_Registry = std::make_unique<entt::registry>();

	LoadResources();
	CreateInitialEntities();

	std::cout << "=== Game Initialized ===" << std::endl;
}

void GameApp::LoadResources() {
	std::cout << "Loading resources..." << std::endl;

	ResourceManager* resources = m_RenderSystem->GetResourceManager();

	// Create primitive meshes
	resources->CreatePlane("ground", 20.0f, 20.0f);
	resources->CreateCube("cube", 1.0f);
	resources->CreateQuad("ui_quad");

	// Load 3D models (uncomment when you have models)
	// resources->LoadMeshAs("models/conveyor.glb", "conveyor");
	// resources->LoadMeshAs("models/assembler.glb", "assembler");
	// resources->LoadMeshAs("models/furnace.glb", "furnace");

	std::cout << "Resources loaded" << std::endl;
}

void GameApp::CreateInitialEntities() {
	std::cout << "Creating initial entities..." << std::endl;

	ResourceManager* resources = m_RenderSystem->GetResourceManager();

	// Create ground plane
	{
		auto entity = m_Registry->create();

		auto& transform = m_Registry->emplace<Transform>(entity);
		transform.position = glm::vec3(0.0f, -0.01f, 0.0f);

		auto& meshComp = m_Registry->emplace<MeshComponent>(entity);
		meshComp.mesh = resources->GetMesh("ground");

		auto& material = m_Registry->emplace<MaterialComponent>(entity);
		material.color = glm::vec4(0.2f, 0.4f, 0.2f, 1.0f);

		m_Registry->emplace<NameTag>(entity, "Ground");
	}

	// Create a grid of buildings as an example
	for (int x = -2; x <= 2; x++) {
		for (int z = -2; z <= 2; z++) {
			if (x == 0 && z == 0) continue; // Leave center empty

			auto entity = m_Registry->create();

			auto& transform = m_Registry->emplace<Transform>(entity);
			transform.position = glm::vec3(x * 2.0f, 0.5f, z * 2.0f);
			transform.scale = glm::vec3(0.8f);

			auto& meshComp = m_Registry->emplace<MeshComponent>(entity);
			meshComp.mesh = resources->GetMesh("cube");

			auto& material = m_Registry->emplace<MaterialComponent>(entity);
			// Vary color based on position
			float r = (x + 2) / 4.0f;
			float b = (z + 2) / 4.0f;
			material.color = glm::vec4(r, 0.5f, b, 1.0f);

			auto& gridPos = m_Registry->emplace<GridPosition>(entity);
			gridPos.x = x;
			gridPos.y = z;

			auto& building = m_Registry->emplace<BuildingComponent>(entity);
			building.type = (x + z) % 2 == 0 ? BuildingType::Assembler : BuildingType::Furnace;
			building.rotation = (x + z) % 4;
		}
	}

	// Create UI elements
	{
		auto entity = m_Registry->create();

		auto& sprite = m_Registry->emplace<SpriteComponent>(entity);
		sprite.position = glm::vec2(20.0f, 20.0f);
		sprite.size = glm::vec2(300.0f, 60.0f);
		sprite.color = glm::vec4(0.1f, 0.1f, 0.1f, 0.8f);
		sprite.layer = 0;

		m_Registry->emplace<NameTag>(entity, "UI Background");
	}

	{
		auto entity = m_Registry->create();

		auto& sprite = m_Registry->emplace<SpriteComponent>(entity);
		sprite.position = glm::vec2(30.0f, 30.0f);
		sprite.size = glm::vec2(280.0f, 40.0f);
		sprite.color = glm::vec4(0.9f, 0.7f, 0.2f, 1.0f);
		sprite.layer = 1;

		m_Registry->emplace<NameTag>(entity, "UI Info Bar");
	}

	std::cout << "Created " << m_Registry->storage<entt::entity>().size() << " entities" << std::endl;
}

void GameApp::HandleInput(float deltaTime) {
	GLFWwindow* window = m_VulkanContext->GetWindow();

	// Camera controls
	float moveSpeed = 10.0f * deltaTime;
	float rotateSpeed = 90.0f * deltaTime; // degrees per second

	// WASD for camera panning
	glm::vec3 moveDir(0.0f);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		moveDir.z -= 1.0f;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		moveDir.z += 1.0f;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		moveDir.x -= 1.0f;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		moveDir.x += 1.0f;
	}

	if (glm::length(moveDir) > 0.0f) {
		moveDir = glm::normalize(moveDir);
		m_CameraTarget += moveDir * moveSpeed;
		m_Camera->SetTarget(m_CameraTarget);
	}

	// Q/E for camera rotation
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
		m_CameraYaw -= rotateSpeed;
		m_Camera->SetIsometricAngle(glm::radians(m_CameraPitch), glm::radians(m_CameraYaw));
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
		m_CameraYaw += rotateSpeed;
		m_Camera->SetIsometricAngle(glm::radians(m_CameraPitch), glm::radians(m_CameraYaw));
	}

	// Mouse wheel for zoom (you'd need to add a scroll callback)
	// R/F for manual zoom
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
		m_CameraDistance -= moveSpeed;
		m_Camera->SetDistance(m_CameraDistance);
	}
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
		m_CameraDistance += moveSpeed;
		m_Camera->SetDistance(m_CameraDistance);
	}

	// ESC to quit
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		m_IsRunning = false;
	}
}

void GameApp::Update(float deltaTime) {
	// Update production buildings
	auto productionView = m_Registry->view<ProductionComponent, BuildingComponent>();
	for (auto entity : productionView) {
		auto& production = productionView.get<ProductionComponent>(entity);
		auto& building = productionView.get<BuildingComponent>(entity);

		if (building.isActive) {
			production.progress += deltaTime;
			if (production.progress >= production.productionTime) {
				production.progress = 0.0f;
				// TODO: Output produced item
			}
		}
	}

	// Update conveyor movement
	auto conveyorView = m_Registry->view<ConveyorComponent>();
	for (auto entity : conveyorView) {
		auto& conveyor = conveyorView.get<ConveyorComponent>(entity);
		// TODO: Move items along belt
	}

	// Rotate buildings slightly for visual effect (just for demo)
	static float rotationTime = 0.0f;
	rotationTime += deltaTime * 0.5f;

	auto rotatingView = m_Registry->view<Transform, BuildingComponent>();
	for (auto entity : rotatingView) {
		auto& transform = rotatingView.get<Transform>(entity);
		// Slight bobbing animation
		transform.position.y = 0.5f + std::sin(rotationTime + transform.position.x) * 0.05f;
	}
}

void GameApp::Render() {
	m_RenderSystem->RenderFrame(*m_Registry, *m_Camera);
}

void GameApp::Shutdown() {
	std::cout << "=== Shutting Down Game ===" << std::endl;

	if (m_Registry) {
		m_Registry->clear();
		m_Registry.reset();
	}

	if (m_Camera) {
		m_Camera.reset();
	}

	if (m_RenderSystem) {
		m_RenderSystem->Shutdown();
		m_RenderSystem.reset();
	}

	if (m_VulkanContext) {
		m_VulkanContext->Shutdown();
		m_VulkanContext.reset();
	}

	std::cout << "=== Game Shut Down ===" << std::endl;
}
