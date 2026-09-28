// ChunkCuller.h
#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "Frustum.h"

class VulkanEngine;
class TerrainBufferPool;

class ChunkCuller {
public:
    void Init(VulkanEngine* engine, TerrainBufferPool* pool, uint32_t maxChunks, uint32_t indicesPerChunk, uint32_t vertsPerChunk);
    void Shutdown(VkDevice device);

    // Dispatches the compute shader, waits for completion, returns how many chunks survived culling
    uint32_t DispatchAndReadCount(const Frustum& frustum);
    // ChunkCuller.h — replace DispatchAndReadCount
    void RecordCullingCommands(VkCommandBuffer commandBuffer, const Frustum& frustum);
    void RecordBarrier(VkCommandBuffer commandBuffer);

    VkBuffer GetDrawCommandBuffer() const { return m_DrawCommandBuffer; }
    VkBuffer GetDrawCountBuffer() const { return m_DrawCountBuffer; }   // now used directly by the GPU, not read back


private:
    VulkanEngine* m_Engine = nullptr;
    TerrainBufferPool* m_Pool = nullptr;

    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;

    VkBuffer m_DrawCommandBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_DrawCommandBufferMemory = VK_NULL_HANDLE;

    VkBuffer m_DrawCountBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_DrawCountBufferMemory = VK_NULL_HANDLE;
    void* m_DrawCountBufferMapped = nullptr;

    uint32_t m_MaxChunks = 0;
    uint32_t m_IndicesPerChunk = 0;
    uint32_t m_VertsPerChunk = 0;
};