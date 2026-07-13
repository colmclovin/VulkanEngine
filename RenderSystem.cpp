#include "RenderSystem.h"
#include "VulkanContext.h"
#include "ResourceManager.h"
#include "MeshRenderer.h"
#include "UIRenderer.h"
#include "Camera3D.h"
#include <entt/entt.hpp>
#include <iostream>

RenderSystem::RenderSystem(VulkanContext* context)
	: m_Context(context)
{
}

RenderSystem::~RenderSystem() {
	Shutdown();
}

void RenderSystem::Init() {
	// Initialize resource manager first
	m_ResourceManager = std::make_unique<ResourceManager>(m_Context);
	m_ResourceManager->Init();

	// Initialize rendering subsystems
	m_MeshRenderer = std::make_unique<MeshRenderer>(m_Context);
	m_MeshRenderer->Init();

	m_UIRenderer = std::make_unique<UIRenderer>(m_Context);
	m_UIRenderer->Init();

	std::cout << "RenderSystem initialized with all subsystems" << std::endl;
}

void RenderSystem::RenderFrame(entt::registry& registry, const Camera3D& camera) {
	// Begin Vulkan frame
	if (!m_Context->BeginFrame()) {
		return; // Skip frame if swapchain needs recreation
	}

	// Render 3D meshes first
	m_MeshRenderer->Render(registry, camera);

	// Render 2D UI on top
	m_UIRenderer->Render(registry);

	// End frame and present
	m_Context->EndFrame();
}

void RenderSystem::Shutdown() {
	if (m_UIRenderer) {
		m_UIRenderer->Shutdown();
		m_UIRenderer.reset();
	}

	if (m_MeshRenderer) {
		m_MeshRenderer->Shutdown();
		m_MeshRenderer.reset();
	}

	if (m_ResourceManager) {
		m_ResourceManager->Shutdown();
		m_ResourceManager.reset();
	}

	std::cout << "RenderSystem shut down" << std::endl;
}
