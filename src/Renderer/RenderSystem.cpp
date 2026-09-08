#include "RenderSystem.h"
#include "QuadRenderer.h"
#include "MeshRenderer.h"
#include "DebugLineRenderer.h"
#include "ImGuiVulkanUtil.h"
#include "../Engine/VulkanEngine.h"
#include "../Audio/AudioEngine.h"
#include "../Helpers/DebugUI.h"
#include <iostream>
#include "../Components/GameSettings.h"
#include "../Components/Components.h"
RenderSystem::RenderSystem(VulkanEngine *engine) : m_Engine(engine) {
}

RenderSystem::~RenderSystem() {
    Shutdown();
}


void RenderSystem::Init() {
    std::cout << "RenderSystem initializing..." << std::endl;
    m_QuadRenderer = std::make_unique<QuadRenderer>(m_Engine);
    m_QuadRenderer->Init();
    
    m_MeshRenderer = std::make_unique<MeshRenderer>(m_Engine);
    m_MeshRenderer->Init();

    m_DebugLineRenderer = std::make_unique<DebugLineRenderer>(m_Engine);
    m_DebugLineRenderer->Init();


    m_ImGuiVulkanUtil = std::make_unique<ImGuiVulkanUtil>(m_Engine);
    m_ImGuiVulkanUtil->Init(m_Engine->GetWindow(), m_Engine->GetInstance(), m_Engine->GetSwapChainFormat(), m_Engine->GetSwapChainImageCount());
        
    m_DebugUI = std::make_unique<DebugUI>();

    m_initialized = true;
    std::cout << "RenderSystem initialized with all subsystems" << std::endl;
}

void RenderSystem::RenderFrame(entt::registry &registry, Camera3D &camera, GameSettings &settings, AudioEngine &audioEngine, entt::entity m_PlayerEntity, entt::entity m_InspectedEntity) {
    m_Engine->SetClearColor(settings.clearColor);   // NEW

    // Render the frame using the quad renderer
    if (!m_Engine->BeginFrame()) {
        ImGui::EndFrame();
        return; // Skip frame if swapchain needs recreation
    }
    
    
    m_ImGuiVulkanUtil->NewFrame();                              // MOVED — now always runs against up-to-date window state
    m_DebugUI->Draw(registry, this, &camera, settings, &audioEngine, m_PlayerEntity, m_InspectedEntity);

    m_MeshRenderer->Render(registry, camera, settings.wireframeMode);
    m_QuadRenderer->Render(registry);



    m_DebugLineRenderer->BeginFrame();

    if (settings.showBoundsDebug) {
        auto view = registry.view<TransformComponent, BoundsComponent>();
        for (auto entity : view) {
            auto &t = view.get<TransformComponent>(entity);
            auto &b = view.get<BoundsComponent>(entity);
            glm::vec3 center = t.Position + glm::vec3(0, b.halfExtents.y, 0);
            m_DebugLineRenderer->AddBox(center - b.halfExtents, center + b.halfExtents, glm::vec3(0, 1, 0));
        }
    }

    if (settings.showGridDebug) {
        // Draw a flat grid across the terrain extent — simplest: lines every GRID_SIZE units
        float gridSize = settings.terrain.cellSize;
        float worldW = settings.terrain.gridWidth * gridSize;
        float worldD = settings.terrain.gridDepth * gridSize;
        for (float x = 0; x <= worldW; x += gridSize) {
            m_DebugLineRenderer->AddLine({ x, 0.1f, 0 }, { x, 0.1f, worldD }, glm::vec3(0.5f));
        }
        for (float z = 0; z <= worldD; z += gridSize) {
            m_DebugLineRenderer->AddLine({ 0, 0.1f, z }, { worldW, 0.1f, z }, glm::vec3(0.5f));
        }
    }

    // after mesh/quad rendering, before ImGui
    VkExtent2D extent = m_Engine->GetSwapChainExtent();
    float aspect = (float)extent.width / extent.height;
    m_DebugLineRenderer->Render(camera, aspect);



    m_ImGuiVulkanUtil->RenderDrawData(m_Engine->GetCurrentCommandBuffer());
    m_Engine->EndFrame();
}

void RenderSystem::Shutdown() {
	if (!m_initialized) {
		return;
	}
    if (m_QuadRenderer) {
        m_QuadRenderer->Shutdown();
        m_QuadRenderer.reset();
    }
    if (m_ImGuiVulkanUtil) {           
        m_ImGuiVulkanUtil->Shutdown();
        m_ImGuiVulkanUtil.reset();
    }


    m_initialized = false;
    std::cout << "RenderSystem shut down" << std::endl;
}

DebugUI* RenderSystem::GetDebugUI() const {
    return m_DebugUI.get();
}
