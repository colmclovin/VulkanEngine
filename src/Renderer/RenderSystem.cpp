#include "RenderSystem.h"
#include "QuadRenderer.h"
#include "../Engine/VulkanEngine.h"
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

    m_initialized = true;
    std::cout << "RenderSystem initialized with all subsystems" << std::endl;
}

void RenderSystem::RenderFrame(entt::registry &registry){//, const Camera3D &camera) {
    // Render the frame using the quad renderer
    if (!m_Engine->BeginFrame()) {
        return; // Skip frame if swapchain needs recreation
    }
    
    
    m_QuadRenderer->Render(registry);


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
    m_initialized = false;
    std::cout << "RenderSystem shut down" << std::endl;
}
