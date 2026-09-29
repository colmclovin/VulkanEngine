// TerrainBufferPool.cpp
#include "TerrainBufferPool.h"
#include "../Engine/VulkanEngine.h"
#include <stdexcept>
#include <iostream>
#include <cstring>

void TerrainBufferPool::Init(VulkanEngine* engine, uint32_t vertsPerChunk, uint32_t indicesPerChunk, uint32_t maxChunks) {
    m_Engine = engine;
    m_VertsPerChunk = vertsPerChunk;
    m_IndicesPerChunk = indicesPerChunk;
    m_MaxChunks = maxChunks;

    VkDevice device = engine->GetDevice();

    VkDeviceSize vertexBufferSize = static_cast<VkDeviceSize>(vertsPerChunk) * maxChunks * sizeof(Vertex);
    VkDeviceSize indexBufferSize = static_cast<VkDeviceSize>(indicesPerChunk) * maxChunks * sizeof(uint32_t);

    // Device-local for performance; we'll upload via staging buffer per-chunk, same pattern as your existing Mesh::UploadToGPU
    engine->CreateBuffer(vertexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,   // ADD STORAGE_BUFFER_BIT
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffer, m_VertexBufferMemory);

    engine->CreateBuffer(indexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_IndexBuffer, m_IndexBufferMemory);

    m_SlotInUse.assign(maxChunks, false);
    for (uint32_t i = 0; i < maxChunks; i++) {
        m_FreeSlots.push(i);
    }

    VkDeviceSize metadataBufferSize = maxChunks * sizeof(ChunkGpuMetadata);
    engine->CreateBuffer(metadataBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_MetadataBuffer, m_MetadataBufferMemory);
    vkMapMemory(device, m_MetadataBufferMemory, 0, metadataBufferSize, 0, &m_MetadataBufferMapped);

    // Initialize every slot as inactive
    ChunkGpuMetadata* metadata = static_cast<ChunkGpuMetadata*>(m_MetadataBufferMapped);
    for (uint32_t i = 0; i < maxChunks; i++) {
        metadata[i].isActive = 0;
    }

    std::cout << "TerrainBufferPool initialized: " << maxChunks << " slots, "
        << (vertexBufferSize / (1024.0 * 1024.0)) << " MB vertex buffer, "
        << (indexBufferSize / (1024.0 * 1024.0)) << " MB index buffer" << std::endl;
}

std::optional<uint32_t> TerrainBufferPool::AllocateSlot() {
    if (m_FreeSlots.empty()) {
        std::cerr << "TerrainBufferPool: out of slots! Increase maxChunks." << std::endl;
        return std::nullopt;
    }
    uint32_t slot = m_FreeSlots.front();
    m_FreeSlots.pop();

    if (m_SlotInUse[slot]) {
        throw std::runtime_error("TerrainBufferPool: allocated a slot that's already marked in-use — allocator bug");
    }
    m_SlotInUse[slot] = true;
    return slot;
}



void TerrainBufferPool::UploadChunkData(uint32_t slot, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
    if (vertices.size() != m_VertsPerChunk || indices.size() != m_IndicesPerChunk) {
        throw std::runtime_error("TerrainBufferPool::UploadChunkData: chunk data size doesn't match fixed slot size");
    }

    VkDevice device = m_Engine->GetDevice();

    // Vertex data
    VkDeviceSize vertexRegionOffset = static_cast<VkDeviceSize>(slot) * m_VertsPerChunk * sizeof(Vertex);
    VkDeviceSize vertexRegionSize = m_VertsPerChunk * sizeof(Vertex);

    VkBuffer vStaging; VkDeviceMemory vStagingMem;
    m_Engine->CreateBuffer(vertexRegionSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vStaging, vStagingMem);
    void* vData;
    vkMapMemory(device, vStagingMem, 0, vertexRegionSize, 0, &vData);
    memcpy(vData, vertices.data(), vertexRegionSize);
    vkUnmapMemory(device, vStagingMem);

    // NOTE: CopyBuffer as you have it likely copies offset 0 to offset 0 — we need offset-aware copying here.
    // If your existing CopyBuffer doesn't support a destination offset, this needs a small extension (see below).
    m_Engine->CopyBufferRegion(vStaging, m_VertexBuffer, vertexRegionSize, 0, vertexRegionOffset);

    vkDestroyBuffer(device, vStaging, nullptr);
    vkFreeMemory(device, vStagingMem, nullptr);

    // Index data — same pattern
    VkDeviceSize indexRegionOffset = static_cast<VkDeviceSize>(slot) * m_IndicesPerChunk * sizeof(uint32_t);
    VkDeviceSize indexRegionSize = m_IndicesPerChunk * sizeof(uint32_t);

    VkBuffer iStaging; VkDeviceMemory iStagingMem;
    m_Engine->CreateBuffer(indexRegionSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, iStaging, iStagingMem);
    void* iData;
    vkMapMemory(device, iStagingMem, 0, indexRegionSize, 0, &iData);
    memcpy(iData, indices.data(), indexRegionSize);
    vkUnmapMemory(device, iStagingMem);

    m_Engine->CopyBufferRegion(iStaging, m_IndexBuffer, indexRegionSize, 0, indexRegionOffset);

    vkDestroyBuffer(device, iStaging, nullptr);
    vkFreeMemory(device, iStagingMem, nullptr);
}

void TerrainBufferPool::Shutdown(VkDevice device) {
    if (m_VertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_VertexBuffer, nullptr);
        vkFreeMemory(device, m_VertexBufferMemory, nullptr);
    }
    if (m_IndexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_IndexBuffer, nullptr);
        vkFreeMemory(device, m_IndexBufferMemory, nullptr);
    }
    if (m_MetadataBuffer != VK_NULL_HANDLE) {   // ADD
        vkDestroyBuffer(device, m_MetadataBuffer, nullptr);
        vkFreeMemory(device, m_MetadataBufferMemory, nullptr);
    }
}

void TerrainBufferPool::UploadMetadata(uint32_t slot, glm::vec3 boundsCenter, float boundsRadius) {
    ChunkGpuMetadata* metadata = static_cast<ChunkGpuMetadata*>(m_MetadataBufferMapped);
    metadata[slot].boundsCenterAndRadius = glm::vec4(boundsCenter, boundsRadius);
    metadata[slot].slot = slot;
    metadata[slot].isActive = 1;
}

void TerrainBufferPool::ClearMetadata(uint32_t slot) {
    ChunkGpuMetadata* metadata = static_cast<ChunkGpuMetadata*>(m_MetadataBufferMapped);
    metadata[slot].isActive = 0;
}

void TerrainBufferPool::FreeSlot(uint32_t slot) {
    if (!m_SlotInUse[slot]) throw std::runtime_error("double-free");
    m_PendingFrees.push_back({ slot, MAX_FRAMES_IN_FLIGHT }); // don't push to m_FreeSlots yet
}

void TerrainBufferPool::ProcessPendingFrees() {
    for (auto it = m_PendingFrees.begin(); it != m_PendingFrees.end();) {
        it->framesRemaining--;
        if (it->framesRemaining <= 0) {
            m_SlotInUse[it->slot] = false;
            m_FreeSlots.push(it->slot); // NOW safe to reuse
            it = m_PendingFrees.erase(it);
        } else {
            ++it;
        }
    }
}

void TerrainBufferPool::InitializeSharedIndices() {
    std::vector<uint32_t> indicesPerChunkPattern;
    uint32_t res = static_cast<uint32_t>(std::sqrt(m_VertsPerChunk));   // 32, assuming square chunks
    std::cout << "InitializeSharedIndices: m_VertsPerChunk=" << m_VertsPerChunk << " computed res=" << res
        << " m_IndicesPerChunk=" << m_IndicesPerChunk << std::endl;
    for (uint32_t z = 0; z < res - 1; z++) {
        for (uint32_t x = 0; x < res - 1; x++) {
            uint32_t topLeft = z * res + x;
            uint32_t topRight = topLeft + 1;
            uint32_t bottomLeft = (z + 1) * res + x;
            uint32_t bottomRight = bottomLeft + 1;

            indicesPerChunkPattern.push_back(topLeft);
            indicesPerChunkPattern.push_back(bottomLeft);
            indicesPerChunkPattern.push_back(topRight);
            indicesPerChunkPattern.push_back(topRight);
            indicesPerChunkPattern.push_back(bottomLeft);
            indicesPerChunkPattern.push_back(bottomRight);
        }
    }

    // Each slot's indices are IDENTICAL in relative structure (0-indexed within that slot's own vertex range),
    // since the graphics pipeline's indirect draw command specifies a per-draw vertexOffset that shifts
    // these relative indices into the correct absolute vertex range automatically.
    for (uint32_t slot = 0; slot < m_MaxChunks; slot++) {
        VkDeviceSize indexRegionOffset = static_cast<VkDeviceSize>(slot) * m_IndicesPerChunk * sizeof(uint32_t);
        VkDeviceSize indexRegionSize = m_IndicesPerChunk * sizeof(uint32_t);

        VkBuffer staging; VkDeviceMemory stagingMem;
        m_Engine->CreateBuffer(indexRegionSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging, stagingMem);
        void* data;
        vkMapMemory(m_Engine->GetDevice(), stagingMem, 0, indexRegionSize, 0, &data);
        memcpy(data, indicesPerChunkPattern.data(), indexRegionSize);
        vkUnmapMemory(m_Engine->GetDevice(), stagingMem);

        m_Engine->CopyBufferRegion(staging, m_IndexBuffer, indexRegionSize, 0, indexRegionOffset);

        vkDestroyBuffer(m_Engine->GetDevice(), staging, nullptr);
        vkFreeMemory(m_Engine->GetDevice(), stagingMem, nullptr);
    }

    std::cout << "TerrainBufferPool: initialized shared indices for all " << m_MaxChunks << " slots" << std::endl;
}