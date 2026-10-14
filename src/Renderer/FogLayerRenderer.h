#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

class VulkanEngine;
class FogTexture;

class FogLayerRenderer {
public:
    explicit FogLayerRenderer(VulkanEngine *engine) : m_Engine(engine) {}
    void Init(FogTexture *fogTex);
    void Record(VkCommandBuffer cmd, const glm::mat4 &viewProj);
    void Shutdown();

private:
    VulkanEngine *m_Engine;
    VkDescriptorSetLayout m_SetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_Pool = VK_NULL_HANDLE;
    VkDescriptorSet m_Set = VK_NULL_HANDLE;
    VkPipelineLayout m_Layout = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
};