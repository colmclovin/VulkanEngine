// TreeInstanceBufferPool.cpp
#include "TreeInstanceBufferPool.h"
#include "../Engine/VulkanEngine.h"
#include <stdexcept>
#include <iostream>
#include <cstring>

void TreeInstanceBufferPool::Init(VulkanEngine* engine, uint32_t maxTreesPerChunk, uint32_t maxChunks) {
    m_Engine = engine;
    m_MaxTreesPerChunk = maxTreesPerChunk;
    m_MaxChunks = maxChunks;

    VkDevice device = engine->GetDevice();

    VkDeviceSize instanceBufferSize = static_cast<VkDeviceSize>(maxTreesPerChunk) * maxChunks * sizeof(TreeInstanceGpuData);
    // Host-visible since tree positions are generated on CPU (worker thread) and uploaded directly — no compute-write needed here,
    // unlike terrain, which generates height/color entirely on GPU. This is a much simpler upload path.
    engine->CreateBuffer(instanceBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_InstanceBuffer, m_InstanceBufferMemory);
    vkMapMemory(device, m_InstanceBufferMemory, 0, instanceBufferSize, 0, &m_InstanceBufferMapped);   // ADD — persistent map

    VkDeviceSize metadataBufferSize = maxChunks * sizeof(TreeChunkGpuMetadata);
    engine->CreateBuffer(metadataBufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,   // ADD TRANSFER_DST_BIT
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_MetadataBuffer, m_MetadataBufferMemory);
    vkMapMemory(device, m_MetadataBufferMemory, 0, metadataBufferSize, 0, &m_MetadataBufferMapped);

    TreeChunkGpuMetadata* metadata = static_cast<TreeChunkGpuMetadata*>(m_MetadataBufferMapped);
    for (uint32_t i = 0; i < maxChunks; i++) metadata[i].isActive = 0;

    m_SlotInUse.assign(maxChunks, false);
    for (uint32_t i = 0; i < maxChunks; i++) m_FreeSlots.push(i);

    std::cout << "TreeInstanceBufferPool initialized: " << maxChunks << " slots, "
        << (instanceBufferSize / (1024.0 * 1024.0)) << " MB instance buffer" << std::endl;
}

std::optional<uint32_t> TreeInstanceBufferPool::AllocateSlot() {
    if (m_FreeSlots.empty()) {
        std::cout << "TreeInstanceBufferPool: out of slots! maxChunks=" << m_MaxChunks << std::endl;   // ADD
        return std::nullopt;
    }
    uint32_t slot = m_FreeSlots.front();
    m_FreeSlots.pop();
    m_SlotInUse[slot] = true;
    return slot;
}

void TreeInstanceBufferPool::FreeSlot(uint32_t slot) {
    if (!m_SlotInUse[slot]) throw std::runtime_error("TreeInstanceBufferPool: double-free on slot " + std::to_string(slot));
    m_PendingFrees.push_back({ slot, MAX_FRAMES_IN_FLIGHT });
}

void TreeInstanceBufferPool::ProcessPendingFrees() {
    for (auto it = m_PendingFrees.begin(); it != m_PendingFrees.end(); ) {
        it->framesRemaining--;
        if (it->framesRemaining <= 0) {
            m_SlotInUse[it->slot] = false;
            m_FreeSlots.push(it->slot);
            it = m_PendingFrees.erase(it);
        }
        else {
            ++it;
        }
    }
}

void TreeInstanceBufferPool::UploadChunkTrees(uint32_t slot, const std::vector<glm::vec3>& treePositions,
    glm::vec3 boundsCenter, float boundsRadius) {
    if (treePositions.size() > m_MaxTreesPerChunk) {
        throw std::runtime_error("TreeInstanceBufferPool: chunk has more trees than max capacity");
    }

    // Directly write into the host-visible instance buffer at this slot's region — no staging needed,
    // since the buffer is host-visible/coherent and this only happens once per chunk load, not per-frame.
    VkDeviceSize offset = static_cast<VkDeviceSize>(slot) * m_MaxTreesPerChunk * sizeof(TreeInstanceGpuData);
    void* mapped;
    vkMapMemory(m_Engine->GetDevice(), m_InstanceBufferMemory, offset, treePositions.size() * sizeof(TreeInstanceGpuData), 0, &mapped);
    TreeInstanceGpuData* data = static_cast<TreeInstanceGpuData*>(m_InstanceBufferMapped);   // CHANGED — use persistent map
    for (size_t i = 0; i < treePositions.size(); i++) {
        data[slot * m_MaxTreesPerChunk + i].positionAndScale = glm::vec4(treePositions[i], 0.3f);
    }
    vkUnmapMemory(m_Engine->GetDevice(), m_InstanceBufferMemory);

    TreeChunkGpuMetadata* metadata = static_cast<TreeChunkGpuMetadata*>(m_MetadataBufferMapped);
    metadata[slot].boundsCenterAndRadius = glm::vec4(boundsCenter, boundsRadius);
    metadata[slot].firstInstance = slot * m_MaxTreesPerChunk;
    metadata[slot].instanceCount = static_cast<uint32_t>(treePositions.size());
    metadata[slot].isActive = 1;
}

void TreeInstanceBufferPool::ClearChunkTrees(uint32_t slot) {
    TreeChunkGpuMetadata* metadata = static_cast<TreeChunkGpuMetadata*>(m_MetadataBufferMapped);
    metadata[slot].isActive = 0;
}

void TreeInstanceBufferPool::Shutdown(VkDevice device) {
    if (m_InstanceBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_InstanceBuffer, nullptr);
        vkFreeMemory(device, m_InstanceBufferMemory, nullptr);
    }
    if (m_MetadataBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_MetadataBuffer, nullptr);
        vkFreeMemory(device, m_MetadataBufferMemory, nullptr);
    }
}

void TreeInstanceBufferPool::UploadBoundsOnly(uint32_t slot, glm::vec3 boundsCenter, float boundsRadius) {
    TreeChunkGpuMetadata* metadata = static_cast<TreeChunkGpuMetadata*>(m_MetadataBufferMapped);
    metadata[slot].boundsCenterAndRadius = glm::vec4(boundsCenter, boundsRadius);
    metadata[slot].firstInstance = slot * m_MaxTreesPerChunk;
    metadata[slot].isActive = 1;
    // instanceCount deliberately left untouched — owned by the generation compute shader
}
void TreeInstanceBufferPool::ClearMetadata(uint32_t slot) {
    TreeChunkGpuMetadata* metadata = static_cast<TreeChunkGpuMetadata*>(m_MetadataBufferMapped);
    metadata[slot].isActive = 0;
}



std::vector<TreeInstanceReadback> TreeInstanceBufferPool::ReadBackChunkInstances(uint32_t slot) const {
    std::vector<TreeInstanceReadback> results;
    if (slot >= m_MaxChunks) return results;

    const TreeChunkGpuMetadata* metadata = static_cast<const TreeChunkGpuMetadata*>(m_MetadataBufferMapped);
    uint32_t count = metadata[slot].instanceCount;
    if (count == 0 || metadata[slot].isActive == 0) return results;

    const glm::vec4* instanceData = static_cast<const glm::vec4*>(m_InstanceBufferMapped);
    for (uint32_t i = 0; i < count; i++) {
        glm::vec4 data = instanceData[slot * m_MaxTreesPerChunk + i];
        results.push_back({ glm::vec3(data.x, data.y, data.z), static_cast<int>(data.w) });
    }
    return results;
}