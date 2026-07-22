#include "Game.h"
#include "../Engine/VulkanEngine.h"
#include "../Renderer/RenderSystem.h"
#include <iostream>
#include <glm/glm.hpp>
#include "../Components/Components.h"
#include "Camera3D.h"
#include "../Components/ModelLoader.h"

Game::Game() {

}
Game::~Game() {
    Shutdown();
}

void Game::Run() {
    Init();
    m_IsRunning = true;
    float lastTime = static_cast<float>(glfwGetTime());

    while (m_IsRunning && !m_VulkanEngine->ShouldClose()) {
        m_VulkanEngine->PollEvents();

        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        HandleInput(deltaTime);
        Update(deltaTime);
        Render();
    }

    Shutdown();
}

void Game::Init() {
    std::cout << "=== Initializing Game ===" << std::endl;
    m_VulkanEngine = std::make_unique<VulkanEngine>();
    m_VulkanEngine->Init("Vulkan Game", 1280, 720);

    m_RenderSystem = std::make_unique<RenderSystem>(m_VulkanEngine.get());
    m_RenderSystem->Init();

	m_Camera = std::make_unique<Camera3D>(glm::vec3(0.0f, 0.0f, 5.0f));
    //glfwSetInputMode(m_VulkanEngine->GetWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);


    m_Registry = std::make_unique<entt::registry>();



    LoadResources();
    CreateInitialEntities();
    m_Initialized = true;
    std::cout << "=== Game Initialized ===" << std::endl;
}


void Game::LoadResources() {
    // Load game resources (textures, meshes, etc.) here
    std::cout << "Loading resources..." << std::endl;

    std::cout << "Resources loaded" << std::endl;
}
void Game::CreateInitialEntities() {
    // Create initial game entities and components here
    std::cout << "Creating initial entities..." << std::endl;

        
        auto entity = m_Registry->create();

        auto &sprite = m_Registry->emplace<SpriteComponent>(entity);
        sprite.transform.Position = glm::vec3(150.0f, 150.0f, 0.0f);
        sprite.transform.Scale = glm::vec3(300.0f, 600.0f, 1.0f);
        sprite.color = glm::vec4(0.3f, 0.2f, 0.1f, 0.9f);
        sprite.layer = 0;

        m_Registry->emplace<NameTag>(entity, "UI Background");
    

        auto meshEntity = m_Registry->create();
        auto loadedMesh = std::make_shared<Mesh>(ModelLoader::LoadModel("Assets/Models/Test1.glb"));
        m_Registry->emplace<MeshComponent>(meshEntity, loadedMesh);
        m_Registry->emplace<TransformComponent>(meshEntity); // defaults: origin, identity rotation, scale 1



}
void Game::HandleInput(float deltaTime) {
    // Handle user input here
    GLFWwindow *window = m_VulkanEngine->GetWindow();

    // Camera controls
    float moveSpeed = 10.0f * deltaTime;
    float rotateSpeed = 90.0f * deltaTime; // degrees per second


    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) m_Camera->ProcessKeyboard(CameraMovement::Forward, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) m_Camera->ProcessKeyboard(CameraMovement::Backward, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) m_Camera->ProcessKeyboard(CameraMovement::Left, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) m_Camera->ProcessKeyboard(CameraMovement::Right, deltaTime);

    //if (glm::length(moveDir) > 0.0f) {
    //    moveDir = glm::normalize(moveDir);
    //    m_CameraTarget += moveDir * moveSpeed;
    //    m_Camera->SetTarget(m_CameraTarget);
    //}

    // Q/E for camera rotation
    //if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
    //    m_CameraYaw -= rotateSpeed;
    //    m_Camera->SetIsometricAngle(glm::radians(m_CameraPitch), glm::radians(m_CameraYaw));
    //}
    //if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
    //    m_CameraYaw += rotateSpeed;
    //    m_Camera->SetIsometricAngle(glm::radians(m_CameraPitch), glm::radians(m_CameraYaw));
    //}

    // Mouse wheel for zoom (you'd need to add a scroll callback)
    // R/F for manual zoom
    //if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
    //    m_CameraDistance -= moveSpeed;
    //    m_Camera->SetDistance(m_CameraDistance);
    //}
    //if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
    //    m_CameraDistance += moveSpeed;
    //    m_Camera->SetDistance(m_CameraDistance);
    //}

    // ESC to quit
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        m_IsRunning = false;
    }
}
void Game::Update(float deltaTime) {
    // Update game logic here
}

void Game::Render() {
    m_RenderSystem->RenderFrame(*m_Registry, *m_Camera);
}
void Game::Shutdown() {
    std::cout << "=== Shutting Down Game ===" << std::endl;
	if (!m_Initialized) {
		std::cout << "Game was not initialized, skipping shutdown." << std::endl;
		return;
	}   

    auto meshView = m_Registry->view<MeshComponent>();
    for (auto entity : meshView) {
        auto& meshComp = meshView.get<MeshComponent>(entity);
        if (meshComp.mesh) {
            meshComp.mesh->DestroyGPUResources(m_VulkanEngine->GetDevice());
        }
    }

    if (m_Registry) {
        m_Registry->clear();
        m_Registry.reset();
    }
    if (m_RenderSystem) {
        m_RenderSystem->Shutdown();
        m_RenderSystem.reset();
    }
    if (m_VulkanEngine) {
        m_VulkanEngine->Shutdown();
        m_VulkanEngine.reset();
    }
    m_Initialized = false;
    std::cout << "=== Game Shut Down ===" << std::endl;
}