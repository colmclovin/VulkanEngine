#include "RenderSystem.h"
#include "QuadRenderer.h"
#include "MeshRenderer.h"
#include "ImGuiVulkanUtil.h"
#include "../Engine/VulkanEngine.h"
#include "../Helpers/DebugUI.h"
#include <iostream>

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

    m_ImGuiVulkanUtil = std::make_unique<ImGuiVulkanUtil>(m_Engine);
    m_ImGuiVulkanUtil->Init(m_Engine->GetWindow(), m_Engine->GetInstance(), m_Engine->GetSwapChainFormat(), m_Engine->GetSwapChainImageCount());
        
    m_DebugUI = std::make_unique<DebugUI>();

    m_initialized = true;
    std::cout << "RenderSystem initialized with all subsystems" << std::endl;
}

void RenderSystem::RenderFrame(entt::registry &registry, const Camera3D &camera) {
    // Render the frame using the quad renderer
    if (!m_Engine->BeginFrame()) {
        return; // Skip frame if swapchain needs recreation
    }

    m_ImGuiVulkanUtil->NewFrame();   
    m_DebugUI->Draw(registry, this);

    m_MeshRenderer->Render(registry, camera);
    m_QuadRenderer->Render(registry);
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
