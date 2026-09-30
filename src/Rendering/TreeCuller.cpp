#include "TreeCuller.h"
#include "TreeInstanceBufferPool.h"
#include "../Engine/VulkanEngine.h"
#include "GpuDrawCommand.h"
#include <stdexcept>
#include <iostream>

void TreeCuller::Init(VulkanEngine* engine, TreeInstanceBufferPool* pool, uint32_t maxChunks, const std::vector<TreeSubmeshInfo>& submeshes) {
    m_Engine = engine;
    m_Pool = pool;
    m_MaxChunks = maxChunks;
    m_Submeshes = submeshes;

    VkDevice device = engine->GetDevice();
    uint32_t numSubmeshes = static_cast<uint32_t>(submeshes.size());

    m_DrawCommandBuffers.resize(numSubmeshes);
    m_DrawCommandMemories.resize(numSubmeshes);
    m_DrawCountBuffers.resize(numSubmeshes);
    m_DrawCountMemories.resize(numSubmeshes);

    for (uint32_t i = 0; i < numSubmeshes; i++) {
        VkDeviceSize drawCmdBufferSize = maxChunks * sizeof(GpuDrawCommand);
        engine->CreateBuffer(drawCmdBufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DrawCommandBuffers[i], m_DrawCommandMemories[i]);

        engine->CreateBuffer(sizeof(uint32_t),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DrawCountBuffers[i], m_DrawCountMemories[i]);
    }

    // Descriptor layout: binding 0 = metadata, then one draw-command + one draw-count binding PER SUBMESH
    // For 2 submeshes: binding 0=metadata, 1=drawCmd[0], 2=drawCount[0], 3=drawCmd[1], 4=drawCount[1]
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.push_back({ 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr });
    for (uint32_t i = 0; i < numSubmeshes; i++) {
        bindings.push_back({ 1 + i * 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr });
        bindings.push_back({ 2 + i * 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr });
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_DescriptorSetLayout);

    VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<uint32_t>(bindings.size()) };
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

    std::vector<VkDescriptorBufferInfo> bufferInfos;
    bufferInfos.reserve(bindings.size());
    bufferInfos.push_back({ pool->GetMetadataBuffer(), 0, VK_WHOLE_SIZE });
    for (uint32_t i = 0; i < numSubmeshes; i++) {
        bufferInfos.push_back({ m_DrawCommandBuffers[i], 0, VK_WHOLE_SIZE });
        bufferInfos.push_back({ m_DrawCountBuffers[i], 0, VK_WHOLE_SIZE });
    }

    std::vector<VkWriteDescriptorSet> writes(bindings.size());
    for (size_t i = 0; i < bindings.size(); i++) {
        writes[i] = {};
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = m_DescriptorSet;
        writes[i].dstBinding = bindings[i].binding;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[i].descriptorCount = 1;
        writes[i].pBufferInfo = &bufferInfos[i];
    }
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

    struct PushData {
        glm::vec4 planes[6];
        uint32_t maxChunks;
        uint32_t numSubmeshes;
        glm::uvec2 submeshInfo[4];   // supports up to 4 submeshes; x=indexCount, y=firstIndex
    };
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(PushData);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushRange;
    vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout);

    auto shaderCode = engine->ReadFile("Shaders/tree_cull_comp.spv");
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
        throw std::runtime_error("Failed to create tree culling pipeline");
    }
    vkDestroyShaderModule(device, shaderModule, nullptr);

    std::cout << "TreeCuller initialized (" << numSubmeshes << " submeshes)" << std::endl;
}

void TreeCuller::RecordCullingCommands(VkCommandBuffer commandBuffer, const Frustum& frustum) {
    for (auto& buf : m_DrawCountBuffers) {
        vkCmdFillBuffer(commandBuffer, buf, 0, sizeof(uint32_t), 0);
    }

    std::vector<VkBufferMemoryBarrier> fillBarriers(m_DrawCountBuffers.size());
    for (size_t i = 0; i < fillBarriers.size(); i++) {
        fillBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        fillBarriers[i].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        fillBarriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        fillBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        fillBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        fillBarriers[i].buffer = m_DrawCountBuffers[i];
        fillBarriers[i].offset = 0;
        fillBarriers[i].size = sizeof(uint32_t);
    }
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 0, nullptr, static_cast<uint32_t>(fillBarriers.size()), fillBarriers.data(), 0, nullptr);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout, 0, 1, &m_DescriptorSet, 0, nullptr);

    struct PushData {
        glm::vec4 planes[6];
        uint32_t maxChunks;
        uint32_t numSubmeshes;
        glm::uvec2 submeshInfo[4];
    } pushData{};
    for (int i = 0; i < 6; i++) pushData.planes[i] = frustum.planes[i];
    pushData.maxChunks = m_MaxChunks;
    pushData.numSubmeshes = static_cast<uint32_t>(m_Submeshes.size());
    for (size_t i = 0; i < m_Submeshes.size() && i < 4; i++) {
        pushData.submeshInfo[i] = glm::uvec2(m_Submeshes[i].indexCount, m_Submeshes[i].firstIndex);
    }

    vkCmdPushConstants(commandBuffer, m_PipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushData), &pushData);

    uint32_t groupCount = (m_MaxChunks + 63) / 64;
    vkCmdDispatch(commandBuffer, groupCount, 1, 1);
}

void TreeCuller::RecordBarrier(VkCommandBuffer commandBuffer) {
    std::vector<VkBufferMemoryBarrier> barriers;
    for (size_t i = 0; i < m_DrawCommandBuffers.size(); i++) {
        VkBufferMemoryBarrier b{};
        b.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        b.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        b.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.buffer = m_DrawCommandBuffers[i];
        b.offset = 0;
        b.size = VK_WHOLE_SIZE;
        barriers.push_back(b);
        b.buffer = m_DrawCountBuffers[i];
        barriers.push_back(b);
    }
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
        0, 0, nullptr, static_cast<uint32_t>(barriers.size()), barriers.data(), 0, nullptr);
}

void TreeCuller::Shutdown(VkDevice device) {
    if (m_Pipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_Pipeline, nullptr);
    if (m_PipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
    if (m_DescriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
    if (m_DescriptorSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
    for (size_t i = 0; i < m_DrawCommandBuffers.size(); i++) {
        vkDestroyBuffer(device, m_DrawCommandBuffers[i], nullptr);
        vkFreeMemory(device, m_DrawCommandMemories[i], nullptr);
        vkDestroyBuffer(device, m_DrawCountBuffers[i], nullptr);
        vkFreeMemory(device, m_DrawCountMemories[i], nullptr);
    }
}