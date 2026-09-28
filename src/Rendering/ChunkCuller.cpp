// ChunkCuller.cpp
#include "ChunkCuller.h"
#include "TerrainBufferPool.h"
#include "../Engine/VulkanEngine.h"
#include <stdexcept>
#include <cstring>
#include <iostream>
#include "GpuDrawCommand.h"


// ChunkCuller.cpp
void ChunkCuller::Init(VulkanEngine* engine, TerrainBufferPool* pool, uint32_t maxChunks, uint32_t indicesPerChunk, uint32_t vertsPerChunk) {
    m_Engine = engine;
    m_Pool = pool;
    m_MaxChunks = maxChunks;
    m_IndicesPerChunk = indicesPerChunk;
    m_VertsPerChunk = vertsPerChunk;

    VkDevice device = engine->GetDevice();

    // Draw command buffer — sized for the worst case (every chunk survives culling)
    VkDeviceSize drawCmdBufferSize = maxChunks * sizeof(GpuDrawCommand);
    engine->CreateBuffer(drawCmdBufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DrawCommandBuffer, m_DrawCommandBufferMemory);

    // Draw count buffer — device-local now; the CPU never reads it back, only vkCmdFillBuffer
    // resets it and the compute shader/indirect draw both operate on it entirely on the GPU.
    engine->CreateBuffer(sizeof(uint32_t),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DrawCountBuffer, m_DrawCountBufferMemory);

    // Descriptor set layout: metadata (binding 0), draw commands (binding 1), draw count (binding 2)
    VkDescriptorSetLayoutBinding bindings[3]{};
    bindings[0] = { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };
    bindings[1] = { 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };
    bindings[2] = { 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 3;
    layoutInfo.pBindings = bindings;
    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_DescriptorSetLayout);

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = 3;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_DescriptorPool);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_DescriptorSetLayout;
    vkAllocateDescriptorSets(device, &allocInfo, &m_DescriptorSet);

    VkDescriptorBufferInfo metadataInfo{ pool->GetMetadataBuffer(), 0, VK_WHOLE_SIZE };
    VkDescriptorBufferInfo drawCmdInfo{ m_DrawCommandBuffer, 0, VK_WHOLE_SIZE };
    VkDescriptorBufferInfo drawCountInfo{ m_DrawCountBuffer, 0, VK_WHOLE_SIZE };

    VkWriteDescriptorSet writes[3]{};
    writes[0] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_DescriptorSet, 0, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &metadataInfo, nullptr };
    writes[1] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_DescriptorSet, 1, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &drawCmdInfo, nullptr };
    writes[2] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_DescriptorSet, 2, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &drawCountInfo, nullptr };
    vkUpdateDescriptorSets(device, 3, writes, 0, nullptr);

    // Push constants: 6 frustum planes + 3 uints
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(glm::vec4) * 6 + sizeof(uint32_t) * 3;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushRange;
    vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout);

    auto shaderCode = engine->ReadFile("Shaders/chunk_cull_comp.spv");
    VkShaderModule shaderModule = engine->CreateShaderModule(shaderCode);

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = shaderModule;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = stageInfo;
    pipelineInfo.layout = m_PipelineLayout;

    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create chunk culling compute pipeline");
    }
    vkDestroyShaderModule(device, shaderModule, nullptr);

    std::cout << "ChunkCuller initialized" << std::endl;
}

uint32_t ChunkCuller::DispatchAndReadCount(const Frustum& frustum) {
    // Reset draw count to 0 before dispatch
    *static_cast<uint32_t*>(m_DrawCountBufferMapped) = 0;

    VkCommandBuffer cmd = m_Engine->BeginSingleTimeCommands();

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout, 0, 1, &m_DescriptorSet, 0, nullptr);

    struct PushData {
        glm::vec4 planes[6];
        uint32_t indicesPerChunk;
        uint32_t vertsPerChunk;
        uint32_t maxChunks;
    } pushData;
    for (int i = 0; i < 6; i++) pushData.planes[i] = frustum.planes[i];
    pushData.indicesPerChunk = m_IndicesPerChunk;
    pushData.vertsPerChunk = m_VertsPerChunk;
    pushData.maxChunks = m_MaxChunks;

    vkCmdPushConstants(cmd, m_PipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushData), &pushData);

    uint32_t groupCount = (m_MaxChunks + 63) / 64;   // matches local_size_x = 64
    vkCmdDispatch(cmd, groupCount, 1, 1);

    m_Engine->EndSingleTimeCommands(cmd);   // waits for completion

    return *static_cast<uint32_t*>(m_DrawCountBufferMapped);
}

void ChunkCuller::Shutdown(VkDevice device) {
    if (m_Pipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_Pipeline, nullptr);
    if (m_PipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
    if (m_DescriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
    if (m_DescriptorSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
    if (m_DrawCommandBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_DrawCommandBuffer, nullptr);
        vkFreeMemory(device, m_DrawCommandBufferMemory, nullptr);
    }
    if (m_DrawCountBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_DrawCountBuffer, nullptr);
        vkFreeMemory(device, m_DrawCountBufferMemory, nullptr);
    }
}

// ChunkCuller.cpp
void ChunkCuller::RecordCullingCommands(VkCommandBuffer commandBuffer, const Frustum& frustum) {
    // Reset draw count to 0 — this write happens on the GPU timeline now, via vkCmdFillBuffer,
    // not a CPU-side memcpy, since we no longer keep this buffer host-mapped for readback.
    vkCmdFillBuffer(commandBuffer, m_DrawCountBuffer, 0, sizeof(uint32_t), 0);

    // Barrier: ensure the fill completes before the compute shader reads/writes the same buffer
    VkBufferMemoryBarrier fillBarrier{};
    fillBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    fillBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    fillBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    fillBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    fillBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    fillBarrier.buffer = m_DrawCountBuffer;
    fillBarrier.offset = 0;
    fillBarrier.size = sizeof(uint32_t);
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 0, nullptr, 1, &fillBarrier, 0, nullptr);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout, 0, 1, &m_DescriptorSet, 0, nullptr);

    struct PushData {
        glm::vec4 planes[6];
        uint32_t indicesPerChunk;
        uint32_t vertsPerChunk;
        uint32_t maxChunks;
    } pushData;
    for (int i = 0; i < 6; i++) pushData.planes[i] = frustum.planes[i];
    pushData.indicesPerChunk = m_IndicesPerChunk;
    pushData.vertsPerChunk = m_VertsPerChunk;
    pushData.maxChunks = m_MaxChunks;

    vkCmdPushConstants(commandBuffer, m_PipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushData), &pushData);

    uint32_t groupCount = (m_MaxChunks + 63) / 64;
    vkCmdDispatch(commandBuffer, groupCount, 1, 1);
}

void ChunkCuller::RecordBarrier(VkCommandBuffer commandBuffer) {
    // Ensure compute writes to the draw command / draw count buffers are visible before the
    // graphics pipeline reads them as indirect draw arguments.
    VkBufferMemoryBarrier barriers[2]{};
    barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barriers[0].dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
    barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[0].buffer = m_DrawCommandBuffer;
    barriers[0].offset = 0;
    barriers[0].size = VK_WHOLE_SIZE;

    barriers[1] = barriers[0];
    barriers[1].buffer = m_DrawCountBuffer;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
        0, 0, nullptr, 2, barriers, 0, nullptr);
}