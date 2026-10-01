// TreeInstanceBufferPool.h
#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <queue>
#include <vector>
#include <optional>

class VulkanEngine;

struct TreeInstanceGpuData {
    glm::vec4 positionAndScale;   // xyz = position, w = scale
};

struct TreeChunkGpuMetadata {
    glm::vec4 boundsCenterAndRadius;
    uint32_t firstInstance;   // offset into the instance buffer where this chunk's trees start
    uint32_t instanceCount;   // how many trees this chunk actually has (can be less than max slot capacity)
    uint32_t isActive;
    uint32_t padding;
};

struct TreeInstanceReadback {
    glm::vec3 position;
    int gridIndex;
};

class TreeInstanceBufferPool {
public:
    void Init(VulkanEngine* engine, uint32_t maxTreesPerChunk, uint32_t maxChunks);
    void Shutdown(VkDevice device);
    std::optional<uint32_t> AllocateSlot();
    void FreeSlot(uint32_t slot);
    void ProcessPendingFrees();

    void UploadChunkTrees(uint32_t slot, const std::vector<glm::vec3>& treePositions, glm::vec3 boundsCenter, float boundsRadius);
    void ClearChunkTrees(uint32_t slot);

    VkBuffer GetInstanceBuffer() const { return m_InstanceBuffer; }
    VkBuffer GetMetadataBuffer() const { return m_MetadataBuffer; }
    uint32_t GetMaxTreesPerChunk() const { return m_MaxTreesPerChunk; }
    uint32_t GetMaxChunks() const { return m_MaxChunks; }
    void UploadBoundsOnly(uint32_t slot, glm::vec3 boundsCenter, float boundsRadius);
    void ClearMetadata(uint32_t slot);
    
    std::vector<TreeInstanceReadback> ReadBackChunkInstances(uint32_t slot) const;
private:
    VulkanEngine* m_Engine = nullptr;

    VkBuffer m_InstanceBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_InstanceBufferMemory = VK_NULL_HANDLE;
    void* m_InstanceBufferMapped = nullptr;

    VkBuffer m_MetadataBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_MetadataBufferMemory = VK_NULL_HANDLE;
    void* m_MetadataBufferMapped = nullptr;

    uint32_t m_MaxTreesPerChunk = 0;
    uint32_t m_MaxChunks = 0;

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    struct PendingFree { uint32_t slot; int framesRemaining; };
    std::vector<PendingFree> m_PendingFrees;

    std::queue<uint32_t> m_FreeSlots;
    std::vector<bool> m_SlotInUse;
};