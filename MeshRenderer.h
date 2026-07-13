#pragma once
#include <vulkan/vulkan.h>
#include <entt/entt.hpp>
#include <vector>
#include <glm/glm.hpp>

// Forward declarations
class VulkanContext;
class Camera3D;
class Mesh;

// ============================================================================
// MeshRenderer - Renders 3D meshes with ECS components
// ============================================================================
class MeshRenderer {
public:
	MeshRenderer(VulkanContext* context);
	~MeshRenderer();

	void Init();
	void Render(entt::registry& registry, const Camera3D& camera);
	void Shutdown();

	// Upload mesh to GPU buffers
	void UploadMesh(Mesh& mesh);

private:
	void CreateDescriptorSetLayout();
	void CreatePipeline();
	void CreateUniformBuffers();
	void UpdateUniformBuffer(uint32_t frameIndex, const glm::mat4& view, const glm::mat4& projection);

	VulkanContext* m_Context = nullptr;

	// Pipeline and layout
	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
	VkPipeline m_Pipeline = VK_NULL_HANDLE;

	// Descriptors
	VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> m_DescriptorSets;

	// Uniform buffers (per frame in flight)
	struct CameraUBO {
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 projection;
	};

	std::vector<VkBuffer> m_UniformBuffers;
	std::vector<VkDeviceMemory> m_UniformBuffersMemory;
	std::vector<void*> m_UniformBuffersMapped;

	// Push constants for per-object data
	struct PushConstants {
		alignas(16) glm::mat4 model;
		alignas(16) glm::vec4 color;
	};
};
