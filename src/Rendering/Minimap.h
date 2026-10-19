#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

class VulkanEngine;
class FogTexture;

class Minimap {
public:
    static constexpr int SIZE = 256;

    void Init(VulkanEngine *engine, FogTexture *fogTex);

    void Shutdown();
    void Record(VkCommandBuffer cmd, glm::vec2 center, float halfRange, float seed);

    VkDescriptorSet GetImGuiTexture() const { return m_ImGuiTex; }

private:
    VulkanEngine *m_Engine = nullptr;
    FogTexture *m_FogTex = nullptr;

    VkImage m_Image = VK_NULL_HANDLE;
    VkDeviceMemory m_Mem = VK_NULL_HANDLE;
    VkImageView m_View = VK_NULL_HANDLE;
    VkSampler m_Sampler = VK_NULL_HANDLE;

    VkDescriptorSetLayout m_SetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_Pool = VK_NULL_HANDLE;
    VkDescriptorSet m_Set = VK_NULL_HANDLE;
    VkPipelineLayout m_PipeLayout = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;

    VkDescriptorSet m_ImGuiTex = VK_NULL_HANDLE;


    uint32_t m_MaxTreesPerChunk = 0, m_MaxChunks = 0;
    VkDescriptorSetLayout m_TreeSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_TreePool = VK_NULL_HANDLE;
    VkDescriptorSet m_TreeSet = VK_NULL_HANDLE;
    VkPipelineLayout m_TreePipeLayout = VK_NULL_HANDLE;
    VkPipeline m_TreePipeline = VK_NULL_HANDLE;
};