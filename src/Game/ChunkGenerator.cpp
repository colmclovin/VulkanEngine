// ChunkGenerator.cpp
#include "ChunkGenerator.h"
#include "../Engine/VulkanEngine.h"
#include "../Rendering/TerrainBufferPool.h"
#include <stdexcept>
#include <iostream>

void ChunkGenerator::Init(VulkanEngine* engine, TerrainBufferPool* pool, uint32_t vertexResolution) {
    m_Engine = engine;
    m_VertexResolution = vertexResolution;
    m_PaddedRes = vertexResolution + 2;
    m_Pool = pool;

    VkDevice device = engine->GetDevice();

    uint32_t paddedCount = m_PaddedRes * m_PaddedRes;
    VkDeviceSize paddedBufSize = paddedCount * sizeof(glm::vec4);

    engine->CreateBuffer(paddedBufSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_PaddedPositionBuffer, m_PaddedPositionMemory);
    engine->CreateBuffer(paddedBufSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_PaddedColorBuffer, m_PaddedColorMemory);

    CreatePass1Pipeline();
    CreatePass2Pipeline(pool);   // CHANGED — pass pool through

    std::cout << "ChunkGenerator initialized (vertexResolution=" << vertexResolution << ")" << std::endl;
}

void ChunkGenerator::CreatePass1Pipeline() {
    VkDevice device = m_Engine->GetDevice();

    VkDescriptorSetLayoutBinding bindings[2]{};
    bindings[0] = { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };
    bindings[1] = { 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = bindings;
    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_Pass1DescSetLayout);

    VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 };
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_Pass1DescPool);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_Pass1DescPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_Pass1DescSetLayout;
    vkAllocateDescriptorSets(device, &allocInfo, &m_Pass1DescSet);

    VkDescriptorBufferInfo posInfo{ m_PaddedPositionBuffer, 0, VK_WHOLE_SIZE };
    VkDescriptorBufferInfo colInfo{ m_PaddedColorBuffer, 0, VK_WHOLE_SIZE };
    VkWriteDescriptorSet writes[2]{};
    writes[0] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_Pass1DescSet, 0, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &posInfo, nullptr };
    writes[1] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_Pass1DescSet, 1, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &colInfo, nullptr };
    vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);

    struct Pass1Push {
        float chunkOriginX, chunkOriginZ, cellSize, seed;
        uint32_t vertexResolution, paddedRes;
    };
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(Pass1Push);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_Pass1DescSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushRange;
    vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_Pass1PipelineLayout);

    auto shaderCode = m_Engine->ReadFile("Shaders/chunk_generate_pass1_comp.spv");
    VkShaderModule shaderModule = m_Engine->CreateShaderModule(shaderCode);

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = shaderModule;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = stageInfo;
    pipelineInfo.layout = m_Pass1PipelineLayout;

    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pass1Pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create chunk generation pass 1 pipeline");
    }
    vkDestroyShaderModule(device, shaderModule, nullptr);
}

void ChunkGenerator::CreatePass2Pipeline(TerrainBufferPool* pool) {
    VkDevice device = m_Engine->GetDevice();

    VkDescriptorSetLayoutBinding bindings[3]{};
    bindings[0] = { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };
    bindings[1] = { 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };
    bindings[2] = { 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 3;
    layoutInfo.pBindings = bindings;
    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_Pass2DescSetLayout);

    VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 };
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_Pass2DescPool);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_Pass2DescPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_Pass2DescSetLayout;
    vkAllocateDescriptorSets(device, &allocInfo, &m_Pass2DescSet);

    VkDescriptorBufferInfo posInfo{ m_PaddedPositionBuffer, 0, VK_WHOLE_SIZE };
    VkDescriptorBufferInfo colInfo{ m_PaddedColorBuffer, 0, VK_WHOLE_SIZE };
    VkDescriptorBufferInfo vertOutInfo{ pool->GetVertexBuffer(), 0, VK_WHOLE_SIZE };   // NEW — real vertex buffer

    VkWriteDescriptorSet writes[3]{};
    writes[0] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_Pass2DescSet, 0, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &posInfo, nullptr };
    writes[1] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_Pass2DescSet, 1, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &colInfo, nullptr };
    writes[2] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_Pass2DescSet, 2, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &vertOutInfo, nullptr };
    vkUpdateDescriptorSets(device, 3, writes, 0, nullptr);

    struct Pass2Push {
        uint32_t vertexResolution, paddedRes, vertexOffset;
    };
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(Pass2Push);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_Pass2DescSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushRange;
    vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_Pass2PipelineLayout);

    auto shaderCode = m_Engine->ReadFile("Shaders/chunk_generate_pass2_comp.spv");
    VkShaderModule shaderModule = m_Engine->CreateShaderModule(shaderCode);

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = shaderModule;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = stageInfo;
    pipelineInfo.layout = m_Pass2PipelineLayout;

    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pass2Pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create chunk generation pass 2 pipeline");
    }
    vkDestroyShaderModule(device, shaderModule, nullptr);
}

void ChunkGenerator::RecordGenerateChunk(VkCommandBuffer commandBuffer, uint32_t slot,
    float chunkOriginX, float chunkOriginZ, float cellSize, float seed) {
    struct Pass1Push {
        float chunkOriginX, chunkOriginZ, cellSize, seed;
        uint32_t vertexResolution, paddedRes;
    } pass1Push{ chunkOriginX, chunkOriginZ, cellSize, seed, m_VertexResolution, m_PaddedRes };
   // std::cout << "RecordGenerateChunk: m_VertexResolution=" << m_VertexResolution << " m_PaddedRes=" << m_PaddedRes << std::endl;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pass1Pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pass1PipelineLayout, 0, 1, &m_Pass1DescSet, 0, nullptr);
    vkCmdPushConstants(commandBuffer, m_Pass1PipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(Pass1Push), &pass1Push);

    uint32_t groupsPerAxis = (m_PaddedRes + 7) / 8;   // matches local_size_x/y = 8
    vkCmdDispatch(commandBuffer, groupsPerAxis, groupsPerAxis, 1);

    // Barrier: Pass 1's writes to padded position/color buffers must complete before Pass 2 reads them
    VkBufferMemoryBarrier barriers[2]{};
    barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[0].buffer = m_PaddedPositionBuffer;
    barriers[0].offset = 0;
    barriers[0].size = VK_WHOLE_SIZE;
    barriers[1] = barriers[0];
    barriers[1].buffer = m_PaddedColorBuffer;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 0, nullptr, 2, barriers, 0, nullptr);

    struct Pass2Push {
        uint32_t vertexResolution, paddedRes, vertexOffset;
    } pass2Push{ m_VertexResolution, m_PaddedRes, slot * m_VertexResolution * m_VertexResolution };

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pass2Pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pass2PipelineLayout, 0, 1, &m_Pass2DescSet, 0, nullptr);
    vkCmdPushConstants(commandBuffer, m_Pass2PipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(Pass2Push), &pass2Push);

    uint32_t groupsPerAxisReal = (m_VertexResolution + 7) / 8;
    vkCmdDispatch(commandBuffer, groupsPerAxisReal, groupsPerAxisReal, 1);

    // Barrier: Pass 2's writes to the real vertex buffer must complete before anything (indirect draw) reads it later
    VkBufferMemoryBarrier vertexBarrier{};
    vertexBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    vertexBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    vertexBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    vertexBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vertexBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vertexBarrier.buffer = m_Pool->GetVertexBuffer();   // now resolves correctly
    vertexBarrier.offset = 0;
    vertexBarrier.size = VK_WHOLE_SIZE;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        0, 0, nullptr, 1, &vertexBarrier, 0, nullptr);
}