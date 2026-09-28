#pragma once
#include <vulkan/vulkan.h>

class VulkanEngine;

class ComputeTest {
public:
    void Init(VulkanEngine* engine);
    void RunAndVerify(VulkanEngine* engine);   // dispatches, waits, reads back, prints results
    void Shutdown(VkDevice device);

private:
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;

    VkBuffer m_OutputBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_OutputBufferMemory = VK_NULL_HANDLE;
    void* m_OutputBufferMapped = nullptr;

    static constexpr uint32_t ELEMENT_COUNT = 64;
};