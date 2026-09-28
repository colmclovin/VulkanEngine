// ComputeTest.cpp
#include "ComputeTest.h"
#include "../Engine/VulkanEngine.h"
#include <iostream>
#include <stdexcept>

void ComputeTest::Init(VulkanEngine* engine) {
    VkDevice device = engine->GetDevice();

    // Descriptor set layout: one storage buffer binding
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;
    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_DescriptorSetLayout);

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = 1;

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

    // Storage buffer, host-visible so we can read results back directly (fine for a test; real use will differ)
    VkDeviceSize bufferSize = ELEMENT_COUNT * sizeof(uint32_t);
    engine->CreateBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_OutputBuffer, m_OutputBufferMemory);
    vkMapMemory(device, m_OutputBufferMemory, 0, bufferSize, 0, &m_OutputBufferMapped);

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = m_OutputBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = bufferSize;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = m_DescriptorSet;
    write.dstBinding = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.descriptorCount = 1;
    write.pBufferInfo = &bufferInfo;
    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

    // Pipeline layout + compute pipeline
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;
    vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout);

    auto shaderCode = engine->ReadFile("Shaders/test_compute_comp.spv");
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
        throw std::runtime_error("Failed to create compute pipeline");
    }

    vkDestroyShaderModule(device, shaderModule, nullptr);

    std::cout << "Compute pipeline created successfully" << std::endl;
}

void ComputeTest::RunAndVerify(VulkanEngine* engine) {
    VkCommandBuffer cmd = engine->BeginSingleTimeCommands();   // reuses your existing helper — runs on the graphics queue, assuming it supports compute

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout, 0, 1, &m_DescriptorSet, 0, nullptr);
    vkCmdDispatch(cmd, ELEMENT_COUNT / 64, 1, 1);   // matches local_size_x = 64 in the shader

    engine->EndSingleTimeCommands(cmd);   // this already waits for completion via your existing helper

    // Read back and verify
    uint32_t* results = static_cast<uint32_t*>(m_OutputBufferMapped);
    bool correct = true;
    for (uint32_t i = 0; i < ELEMENT_COUNT; i++) {
        if (results[i] != i * 2) { correct = false; break; }
    }
    std::cout << "Compute test result: " << (correct ? "PASS" : "FAIL") << " — first few values: "
        << results[0] << "," << results[1] << "," << results[2] << "," << results[3] << std::endl;
}

void ComputeTest::Shutdown(VkDevice device) {
    if (m_Pipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_Pipeline, nullptr);
    if (m_PipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
    if (m_DescriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
    if (m_DescriptorSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
    if (m_OutputBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_OutputBuffer, nullptr);
        vkFreeMemory(device, m_OutputBufferMemory, nullptr);
    }
}