// ChunkGenerator.cpp
#include "ChunkGenerator.h"
#include "../Engine/VulkanEngine.h"
#include "../Rendering/TerrainBufferPool.h"
#include <stdexcept>
#include <iostream>
#include <unordered_set>
void ChunkGenerator::Init(VulkanEngine* engine, TerrainBufferPool* pool, TreeInstanceBufferPool* treePool, uint32_t vertexResolution) {
    m_Engine = engine;
    m_VertexResolution = vertexResolution;
    m_PaddedRes = vertexResolution + 2;
    m_Pool = pool;
    m_TreePool = treePool;
    VkDevice device = engine->GetDevice();

    uint32_t paddedCount = m_PaddedRes * m_PaddedRes;
    VkDeviceSize paddedBufSize = paddedCount * sizeof(glm::vec4);

    engine->CreateBuffer(paddedBufSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_PaddedPositionBuffer, m_PaddedPositionMemory);
    engine->CreateBuffer(paddedBufSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_PaddedColorBuffer, m_PaddedColorMemory);

    CreatePass1Pipeline();
    CreatePass2Pipeline(pool);   // CHANGED — pass pool through
	CreateTreeGenPipeline(treePool);   // NEW — pass tree pool through
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
        uint32_t depletionCount;
        glm::vec4 depletionEntriesArr[4];
        uint32_t _padding;   // ADD — explicit padding to match GLSL's actual 96-byte block size
    };
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushRange.offset = 0;
    pushRange.size = 96;

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

void ChunkGenerator::RecordGenerateChunk(VkCommandBuffer commandBuffer, uint32_t terrainSlot, uint32_t treeSlot,
    float chunkOriginX, float chunkOriginZ, float cellSize, float seed,
    const std::vector<glm::vec4>& depletionEntries,
    const std::unordered_set<int>& removedTreeIndices) {
   //std::cout << "About to record tree generation for slot " << treeSlot << std::endl;
    // ===== Pass 1: padded position/color =====
    struct Pass1Push {
        float chunkOriginX, chunkOriginZ, cellSize, seed;
        uint32_t vertexResolution, paddedRes;
        uint32_t depletionCount;
        glm::vec4 depletionEntriesArr[4];   // CHANGED
    } pass1Push{};
    pass1Push.chunkOriginX = chunkOriginX;
    pass1Push.chunkOriginZ = chunkOriginZ;
    pass1Push.cellSize = cellSize;
    pass1Push.seed = seed;
    pass1Push.vertexResolution = m_VertexResolution;
    pass1Push.paddedRes = m_PaddedRes;
    pass1Push.depletionCount = static_cast<uint32_t>(std::min(depletionEntries.size(), size_t(4)));   // CHANGED
    for (uint32_t i = 0; i < pass1Push.depletionCount; i++) {
        pass1Push.depletionEntriesArr[i] = depletionEntries[i];
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pass1Pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pass1PipelineLayout, 0, 1, &m_Pass1DescSet, 0, nullptr);
    vkCmdPushConstants(commandBuffer, m_Pass1PipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(Pass1Push), &pass1Push);

    uint32_t groupsPerAxis = (m_PaddedRes + 7) / 8;
    vkCmdDispatch(commandBuffer, groupsPerAxis, groupsPerAxis, 1);

    VkBufferMemoryBarrier pass1Barriers[2]{};
    pass1Barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    pass1Barriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    pass1Barriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    pass1Barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    pass1Barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    pass1Barriers[0].buffer = m_PaddedPositionBuffer;
    pass1Barriers[0].offset = 0;
    pass1Barriers[0].size = VK_WHOLE_SIZE;
    pass1Barriers[1] = pass1Barriers[0];
    pass1Barriers[1].buffer = m_PaddedColorBuffer;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 0, nullptr, 2, pass1Barriers, 0, nullptr);

    // ===== Pass 2: normals + final vertex write =====
    struct Pass2Push {
        uint32_t vertexResolution, paddedRes, vertexOffset;
    } pass2Push{ m_VertexResolution, m_PaddedRes, terrainSlot * m_VertexResolution * m_VertexResolution };

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pass2Pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pass2PipelineLayout, 0, 1, &m_Pass2DescSet, 0, nullptr);
    vkCmdPushConstants(commandBuffer, m_Pass2PipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(Pass2Push), &pass2Push);

    uint32_t groupsPerAxisReal = (m_VertexResolution + 7) / 8;
    vkCmdDispatch(commandBuffer, groupsPerAxisReal, groupsPerAxisReal, 1);

    VkBufferMemoryBarrier vertexBarrier{};
    vertexBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    vertexBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    vertexBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    vertexBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vertexBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vertexBarrier.buffer = m_Pool->GetVertexBuffer();
    vertexBarrier.offset = 0;
    vertexBarrier.size = VK_WHOLE_SIZE;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        0, 0, nullptr, 1, &vertexBarrier, 0, nullptr);

    // ===== Tree generation =====
    // Reset this slot's instanceCount to 0 before the generation shader accumulates into it
    VkDeviceSize instanceCountFieldOffset = static_cast<VkDeviceSize>(treeSlot) * sizeof(TreeChunkGpuMetadata) + offsetof(TreeChunkGpuMetadata, instanceCount);
    vkCmdFillBuffer(commandBuffer, m_TreePool->GetMetadataBuffer(), instanceCountFieldOffset, sizeof(uint32_t), 0);

    VkBufferMemoryBarrier fillBarrier{};
    fillBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    fillBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    fillBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    fillBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    fillBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    fillBarrier.buffer = m_TreePool->GetMetadataBuffer();
    fillBarrier.offset = instanceCountFieldOffset;
    fillBarrier.size = sizeof(uint32_t);

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 0, nullptr, 1, &fillBarrier, 0, nullptr);

    struct TreeGenPush {
        float chunkOriginX, chunkOriginZ, chunkWorldSize, seed;
        uint32_t chunkSlot, maxTreesPerChunk, samplesPerAxis;
        float sampleSpacing;
        uint32_t removedCount;
        uint32_t removedIndicesArr[16];
    } treePush{};
    treePush.chunkOriginX = chunkOriginX;
    treePush.chunkOriginZ = chunkOriginZ;
    treePush.chunkWorldSize = 32.0f;
    treePush.seed = seed;
    treePush.chunkSlot = treeSlot;
    treePush.maxTreesPerChunk = m_TreePool->GetMaxTreesPerChunk();
    treePush.samplesPerAxis = 4;
    treePush.sampleSpacing = 8.0f;
    treePush.removedCount = static_cast<uint32_t>(std::min(removedTreeIndices.size(), size_t(16)));
    {
        uint32_t i = 0;
        for (int idx : removedTreeIndices) {
            if (i >= 16) break;
            treePush.removedIndicesArr[i++] = static_cast<uint32_t>(idx);
        }
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_TreeGenPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_TreeGenPipelineLayout, 0, 1, &m_TreeGenDescSet, 0, nullptr);
    vkCmdPushConstants(commandBuffer, m_TreeGenPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(TreeGenPush), &treePush);

    uint32_t totalSamples = 4 * 4;
    vkCmdDispatch(commandBuffer, (totalSamples + 63) / 64, 1, 1);


    // Barrier: tree generation's writes must complete before the culling shader reads instance/metadata buffers
    VkBufferMemoryBarrier treeBarriers[2]{};
    treeBarriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    treeBarriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    treeBarriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    treeBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    treeBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    treeBarriers[0].buffer = m_TreePool->GetInstanceBuffer();
    treeBarriers[0].offset = 0;
    treeBarriers[0].size = VK_WHOLE_SIZE;
    treeBarriers[1] = treeBarriers[0];
    treeBarriers[1].buffer = m_TreePool->GetMetadataBuffer();

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 0, nullptr, 2, treeBarriers, 0, nullptr);
}

void ChunkGenerator::CreateTreeGenPipeline(TreeInstanceBufferPool* treePool) {
    VkDevice device = m_Engine->GetDevice();

    VkDescriptorSetLayoutBinding bindings[2]{};
    bindings[0] = { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };   // instance buffer
    bindings[1] = { 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };   // count buffer

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = bindings;
    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_TreeGenDescSetLayout);

    VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 };
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_TreeGenDescPool);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_TreeGenDescPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_TreeGenDescSetLayout;
    vkAllocateDescriptorSets(device, &allocInfo, &m_TreeGenDescSet);

    VkDescriptorBufferInfo instInfo{ treePool->GetInstanceBuffer(), 0, VK_WHOLE_SIZE };
    VkDescriptorBufferInfo metaInfo{ treePool->GetMetadataBuffer(), 0, VK_WHOLE_SIZE };   // CHANGED — was countInfo

    VkWriteDescriptorSet writes[2]{};
    writes[0] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_TreeGenDescSet, 0, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &instInfo, nullptr };
    writes[1] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_TreeGenDescSet, 1, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &metaInfo, nullptr };
    vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);

    struct TreeGenPush {   // MUST match RecordGenerateChunk's TreeGenPush exactly
        float chunkOriginX, chunkOriginZ, chunkWorldSize, seed;
        uint32_t chunkSlot, maxTreesPerChunk, samplesPerAxis;
        float sampleSpacing;
        uint32_t removedCount;
        uint32_t removedIndicesArr[16];
    };
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(TreeGenPush);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_TreeGenDescSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushRange;
    vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_TreeGenPipelineLayout);

    auto shaderCode = m_Engine->ReadFile("Shaders/tree_generate_comp.spv");
    VkShaderModule shaderModule = m_Engine->CreateShaderModule(shaderCode);

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = shaderModule;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = stageInfo;
    pipelineInfo.layout = m_TreeGenPipelineLayout;

    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_TreeGenPipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create tree generation pipeline");
    }
    vkDestroyShaderModule(device, shaderModule, nullptr);
}