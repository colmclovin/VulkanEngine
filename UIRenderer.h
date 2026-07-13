#pragma once
#include <vulkan/vulkan.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <vector>

// Forward declarations
class VulkanContext;
class Mesh;

// ============================================================================
// UIRenderer - Renders 2D UI sprites
// ============================================================================
class UIRenderer {
public:
	UIRenderer(VulkanContext* context);
	~UIRenderer();

	void Init();
	void Render(entt::registry& registry);
	void Shutdown();

private:
	void CreatePipeline();
	void CreateQuadMesh();

	VulkanContext* m_Context = nullptr;

	// Pipeline and layout
	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
	VkPipeline m_Pipeline = VK_NULL_HANDLE;

	// Shared quad mesh for all sprites
	Mesh* m_QuadMesh = nullptr;
	bool m_QuadUploaded = false;

	// Push constants for sprite transforms
	struct SpritePushConstants {
		alignas(16) glm::mat4 transform; // Position, rotation, scale in screen space
		alignas(16) glm::vec4 color;     // Sprite tint color
	};
};
