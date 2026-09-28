// TerrainBufferPool.h
#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <queue>
#include <optional>
#include "Vertex.h"
#include "ChunkGpuMetadata.h"

class VulkanEngine;

class TerrainBufferPool {
public:
    void Init(VulkanEngine* engine, uint32_t vertsPerChunk, uint32_t indicesPerChunk, uint32_t maxChunks);
    void Shutdown(VkDevice device);

    // Returns a slot index, or std::nullopt if the pool is full
    std::optional<uint32_t> AllocateSlot();
    void FreeSlot(uint32_t slot);
    void UploadMetadata(uint32_t slot, glm::vec3 boundsCenter, float boundsRadius);
    void ClearMetadata(uint32_t slot);
    VkBuffer GetMetadataBuffer() const { return m_MetadataBuffer; }
    // Uploads one chunk's vertex/index data into its assigned slot's region of the shared buffers
    void UploadChunkData(uint32_t slot, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

    VkBuffer GetVertexBuffer() const { return m_VertexBuffer; }
    VkBuffer GetIndexBuffer() const { return m_IndexBuffer; }
    uint32_t GetVertsPerChunk() const { return m_VertsPerChunk; }
    uint32_t GetIndicesPerChunk() const { return m_IndicesPerChunk; }
    uint32_t GetMaxChunks() const { return m_MaxChunks; }

private:
    VulkanEngine* m_Engine = nullptr;

    VkBuffer m_VertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_VertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer m_IndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_IndexBufferMemory = VK_NULL_HANDLE;
    VkBuffer m_MetadataBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_MetadataBufferMemory = VK_NULL_HANDLE;
    void* m_MetadataBufferMapped = nullptr;
    uint32_t m_VertsPerChunk = 0;
    uint32_t m_IndicesPerChunk = 0;
    uint32_t m_MaxChunks = 0;

    std::queue<uint32_t> m_FreeSlots;
    std::vector<bool> m_SlotInUse;   // for debugging/asserting — catches double-free or use-after-free early
};