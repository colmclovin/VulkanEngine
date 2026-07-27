#include "Game.h"
#include "../Engine/VulkanEngine.h"
#include "../Renderer/RenderSystem.h"
#include <iostream>
#include <glm/glm.hpp>
#include "../Components/Components.h"
#include "Camera3D.h"
#include "../Components/ModelLoader.h"
#include "../Components/TerrainGenerator.h"
#include "../Helpers/DebugUI.h"
#include "../Components/GameSettings.h"
#include <imgui/imgui.h>
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

    std::cout << "=== Loading Settings ===" << std::endl;
    m_Settings = GameSettings::LoadFromFile("settings.json");
    std::cout << "=== Settings Loaded ===" << std::endl;

    std::cout << "=== Initializing Game ===" << std::endl;
    m_VulkanEngine = std::make_unique<VulkanEngine>();
    m_VulkanEngine->Init("Vulkan Game", 1280, 720);

    m_RenderSystem = std::make_unique<RenderSystem>(m_VulkanEngine.get());
    m_RenderSystem->Init();

     m_VulkanEngine->ChainScrollCallback(); 


	m_Camera = std::make_unique<Camera3D>(glm::vec3(0.0f, 5.0f, 5.0f));
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
    
        m_PlayerEntity = m_Registry->create();
        m_Registry->emplace<TransformComponent>(m_PlayerEntity);
        m_Registry->emplace<PlayerComponent>(m_PlayerEntity);

        auto playerMesh = std::make_shared<Mesh>(ModelLoader::LoadModel("Assets/Models/Test1.glb")); // swap for a real player model later
        m_Registry->emplace<MeshComponent>(m_PlayerEntity, playerMesh);
        m_Registry->emplace<NameTag>(m_PlayerEntity, "Player");

        // Tree
        auto treeEntity = m_Registry->create();
        auto& treeTransform = m_Registry->emplace<TransformComponent>(treeEntity);
        treeTransform.Position = glm::vec3(5.0f, 0.0f, 5.0f);
        m_Registry->emplace<TreeComponent>(treeEntity);

        auto treeMesh = std::make_shared<Mesh>(ModelLoader::LoadModel("Assets/Models/cartoon_lowpoly_trees_blend.glb")); // your tree asset
        m_Registry->emplace<MeshComponent>(treeEntity, treeMesh);
        m_Registry->emplace<NameTag>(treeEntity, "Tree");

        
        auto terrainMesh = TerrainGenerator::GenerateHeightmapTerrain(
            m_Settings.terrain.gridWidth,
            m_Settings.terrain.gridDepth,
            m_Settings.terrain.cellSize,
            m_Settings.terrain.heightScale,
            m_Settings.terrain.noiseScale,
            m_Settings.terrain.seed);

        m_TerrainEntity = m_Registry->create();
        m_Registry->emplace<TransformComponent>(m_TerrainEntity);
        m_Registry->emplace<MeshComponent>(m_TerrainEntity, terrainMesh);
        m_Registry->emplace<NameTag>(m_TerrainEntity, "Terrain");
}
void Game::HandleInput(float deltaTime) {
    GLFWwindow* window = m_VulkanEngine->GetWindow();

    // --- Mode toggle (F2), edge-detected like your F1 UI toggle ---
    static bool f2WasDown = false;
    bool f2IsDown = glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS;
    if (f2IsDown && !f2WasDown) {
        bool nowIso = m_Camera->GetMode() == Camera3D::Mode::Isometric;
        m_Camera->SetMode(nowIso ? Camera3D::Mode::FreeFly : Camera3D::Mode::Isometric);

        // Reset mouse delta tracking so switching modes doesn't cause a sudden jump
        // if the cursor moved while the other mode was inactive.
        m_FirstMouse = true;
    }
    f2WasDown = f2IsDown;

    if (m_Camera->GetMode() == Camera3D::Mode::Isometric) {
        HandleIsoInput(window, deltaTime);
    }
    else {
        HandleFreeFlyInput(window, deltaTime);
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        m_IsRunning = false;
    }
}

void Game::HandleIsoInput(GLFWwindow* window, float deltaTime) {
    static bool qWasDown = false, eWasDown = false, f11WasDown = false;
    bool qIsDown = glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS;
    bool eIsDown = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
    bool f11IsDown = glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS;

    if (qIsDown && !qWasDown) m_Camera->SnapRotateIso(false);
    if (eIsDown && !eWasDown) m_Camera->SnapRotateIso(true);
    if (f11IsDown && !f11WasDown) m_VulkanEngine->ToggleFullscreen();
    
    qWasDown = qIsDown;
    eWasDown = eIsDown;
    f11WasDown = f11IsDown;

    glm::vec3 moveDir(0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveDir.z += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveDir.z -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveDir.x -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveDir.x += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) m_Camera->ProcessKeyboard(CameraMovement::Up, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) m_Camera->ProcessKeyboard(CameraMovement::Down, deltaTime);
    // Game::HandleIsoInput
    float scrollDelta = m_VulkanEngine->GetScrollDelta();
    if (scrollDelta != 0.0f && !ImGui::GetIO().WantCaptureMouse) {
        std::cout << "process iso zoom " << scrollDelta * m_Settings.isoZoomSpeed << std::endl;
        m_Camera->ProcessIsoZoom(scrollDelta * m_Settings.isoZoomSpeed);
    }
    m_VulkanEngine->ResetScrollDelta();
    



    if (glm::length(moveDir) > 0.0f && m_Registry->valid(m_PlayerEntity)) {
        MovePlayer(glm::normalize(moveDir), deltaTime);
    }

    m_Camera->UpdateIso(deltaTime);
}

void Game::MovePlayer(glm::vec3 direction, float deltaTime) {
    auto& transform = m_Registry->get<TransformComponent>(m_PlayerEntity);
    auto& player = m_Registry->get<PlayerComponent>(m_PlayerEntity);

    // Move relative to the camera's current facing, same axis logic as the old iso pan —
    // so "forward" is always "up the screen" regardless of which 45° snap we're on.
    float camYaw = m_Camera->GetIsoYaw();   // needs a small getter — see below
    glm::vec3 forward = glm::normalize(glm::vec3(-cos(glm::radians(camYaw)), 0.0f, -sin(glm::radians(camYaw))));
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
	if (glfwGetKey(m_VulkanEngine->GetWindow(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
		transform.Position += (forward * direction.z + right * direction.x) * player.runSpeed * deltaTime;
	}
	else {
		transform.Position += (forward * direction.z + right * direction.x) * player.moveSpeed * deltaTime;
	}
}

void Game::HandleFreeFlyInput(GLFWwindow* window, float deltaTime) {

    bool sprint = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ||
        (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);


    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) m_Camera->ProcessKeyboard(CameraMovement::Forward, deltaTime, sprint);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) m_Camera->ProcessKeyboard(CameraMovement::Backward, deltaTime, sprint);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) m_Camera->ProcessKeyboard(CameraMovement::Left, deltaTime, sprint);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) m_Camera->ProcessKeyboard(CameraMovement::Right, deltaTime, sprint);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) m_Camera->ProcessKeyboard(CameraMovement::Up, deltaTime, sprint);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) m_Camera->ProcessKeyboard(CameraMovement::Down, deltaTime, sprint);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) m_Camera->ProcessKeyboard(CameraMovement::Shift, deltaTime, sprint);
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    if (m_FirstMouse) {
        m_LastMouseX = xpos;
        m_LastMouseY = ypos;
        m_FirstMouse = false;
    }
    float deltaX = static_cast<float>(xpos - m_LastMouseX);
    float deltaY = static_cast<float>(m_LastMouseY - ypos);
    m_LastMouseX = xpos;
    m_LastMouseY = ypos;

    if (!ImGui::GetIO().WantCaptureMouse) {
        m_Camera->ProcessMouseMovement(deltaX, deltaY);
    }
}
void Game::Update(float deltaTime) {
    if (m_Registry->valid(m_PlayerEntity)) {
        auto& transform = m_Registry->get<TransformComponent>(m_PlayerEntity);
        m_Camera->SetIsoTarget(transform.Position);
    }

    if (m_RenderSystem->GetDebugUI()->ConsumeRegenerateRequest()) {
        m_VulkanEngine->WaitIdle();   // ensure GPU is done with the old terrain buffers first

        auto oldTerrainMesh = m_Registry->get<MeshComponent>(m_TerrainEntity).mesh;
        oldTerrainMesh->DestroyGPUResources(m_VulkanEngine->GetDevice());

        auto newTerrainMesh = TerrainGenerator::GenerateHeightmapTerrain(
            m_Settings.terrain.gridWidth, m_Settings.terrain.gridDepth,
            m_Settings.terrain.cellSize, m_Settings.terrain.heightScale,
            m_Settings.terrain.noiseScale, m_Settings.terrain.seed);

        m_Registry->replace<MeshComponent>(m_TerrainEntity, newTerrainMesh);
    }
}

void Game::Render() {
    m_RenderSystem->RenderFrame(*m_Registry, *m_Camera, m_Settings);
}
void Game::Shutdown() {
    std::cout << "=== Shutting Down Game ===" << std::endl;
	
    std::cout << "=== Saving Settings ===" << std::endl;
    m_Settings.SaveToFile("settings.json");
    std::cout << "=== Settings Saved ===" << std::endl;
    
    if (!m_Initialized) {
		std::cout << "Game was not initialized, skipping shutdown." << std::endl;
		return;
	}  


    if (m_VulkanEngine) {
        m_VulkanEngine->WaitIdle();   
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