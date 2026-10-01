// ChunkGenerator.h
#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "../Rendering/TreeInstanceBufferPool.h"
#include <unordered_set>

class VulkanEngine;
class TerrainBufferPool;
class TreeInstanceBufferPool;

class ChunkGenerator {
public:
    void Init(VulkanEngine* engine, TerrainBufferPool* pool, TreeInstanceBufferPool* treePool, uint32_t vertexResolution);
    void Shutdown();

    void RecordGenerateChunk(VkCommandBuffer commandBuffer, uint32_t terrainSlot, uint32_t treeSlot,
        float chunkOriginX, float chunkOriginZ, float cellSize, float seed,
        const std::vector<glm::vec4>& depletionEntries,
        const std::unordered_set<int>& removedTreeIndices);

private:
    void CreatePass1Pipeline();
    void CreatePass2Pipeline(TerrainBufferPool* pool);   // CHANGED — needs pool to write binding 2
    void CreateTreeGenPipeline(TreeInstanceBufferPool* treePool);   // NEW

    VulkanEngine* m_Engine = nullptr;
    uint32_t m_VertexResolution = 0;
    uint32_t m_PaddedRes = 0;
    TerrainBufferPool* m_Pool = nullptr;   // stored during Init for later use
    TreeInstanceBufferPool* m_TreePool = nullptr;   // NEW

    VkPipeline m_Pass1Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_Pass1PipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_Pass1DescSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_Pass1DescPool = VK_NULL_HANDLE;
    VkDescriptorSet m_Pass1DescSet = VK_NULL_HANDLE;

    VkPipeline m_Pass2Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_Pass2PipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_Pass2DescSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_Pass2DescPool = VK_NULL_HANDLE;
    VkDescriptorSet m_Pass2DescSet = VK_NULL_HANDLE;

    // NEW — tree generation pipeline
    VkPipeline m_TreeGenPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_TreeGenPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_TreeGenDescSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_TreeGenDescPool = VK_NULL_HANDLE;
    VkDescriptorSet m_TreeGenDescSet = VK_NULL_HANDLE;

    VkBuffer m_PaddedPositionBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_PaddedPositionMemory = VK_NULL_HANDLE;
    VkBuffer m_PaddedColorBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_PaddedColorMemory = VK_NULL_HANDLE;




};