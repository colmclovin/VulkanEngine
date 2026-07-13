#pragma once
#include <memory>
#include <entt/entt.hpp>

// Forward declarations
class VulkanContext;
class ResourceManager;
class MeshRenderer;
class UIRenderer;
class Camera3D;

// ============================================================================
// RenderSystem - Main coordinator for all rendering subsystems
// ============================================================================
class RenderSystem {
public:
	RenderSystem(VulkanContext* context);
	~RenderSystem();

	void Init();
	void RenderFrame(entt::registry& registry, const Camera3D& camera);
	void Shutdown();

	// Access to subsystems
	ResourceManager* GetResourceManager() const { return m_ResourceManager.get(); }
	MeshRenderer* GetMeshRenderer() const { return m_MeshRenderer.get(); }
	UIRenderer* GetUIRenderer() const { return m_UIRenderer.get(); }

private:
	VulkanContext* m_Context = nullptr;

	// Rendering subsystems
	std::unique_ptr<ResourceManager> m_ResourceManager;
	std::unique_ptr<MeshRenderer> m_MeshRenderer;
	std::unique_ptr<UIRenderer> m_UIRenderer;
};
