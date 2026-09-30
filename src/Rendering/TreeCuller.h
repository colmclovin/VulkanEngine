#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include "Frustum.h"

class VulkanEngine;
class TreeInstanceBufferPool;

struct TreeSubmeshInfo {
    uint32_t indexCount;
    uint32_t firstIndex;
};


class TreeCuller {
public:
    void Init(VulkanEngine* engine, TreeInstanceBufferPool* pool, uint32_t maxChunks, const std::vector<TreeSubmeshInfo>& submeshes);
    void Shutdown(VkDevice device);

    void RecordCullingCommands(VkCommandBuffer commandBuffer, const Frustum& frustum);
    void RecordBarrier(VkCommandBuffer commandBuffer);

    uint32_t GetSubmeshCount() const { return static_cast<uint32_t>(m_Submeshes.size()); }
    VkBuffer GetDrawCommandBuffer(uint32_t submeshIndex) const { return m_DrawCommandBuffers[submeshIndex]; }
    VkBuffer GetDrawCountBuffer(uint32_t submeshIndex) const { return m_DrawCountBuffers[submeshIndex]; }

private:
    VulkanEngine* m_Engine = nullptr;
    TreeInstanceBufferPool* m_Pool = nullptr;
    std::vector<TreeSubmeshInfo> m_Submeshes;

    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;

    // One pair of buffers PER SUBMESH — index 0 = submesh 0's draws, index 1 = submesh 1's draws, etc.
    std::vector<VkBuffer> m_DrawCommandBuffers;
    std::vector<VkDeviceMemory> m_DrawCommandMemories;
    std::vector<VkBuffer> m_DrawCountBuffers;
    std::vector<VkDeviceMemory> m_DrawCountMemories;

    uint32_t m_MaxChunks = 0;
};