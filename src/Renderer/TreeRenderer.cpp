#include "TreeRenderer.h"
#include "../Engine/VulkanEngine.h"
#include "../Rendering/LightingUBO.h"
#include "ShadowMap.h"
#include "../Game/Camera3D.h"
#include "../World/DayNightCycle.h"
#include <stdexcept>
#include <iostream>
#include "../Rendering/GpuDrawCommand.h"
#include "../Rendering/Vertex.h"

TreeRenderer::TreeRenderer(VulkanEngine* engine) : m_Engine(engine) {}

void TreeRenderer::Init(uint32_t maxTreesPerChunk, uint32_t maxChunks, ShadowMap* shadowMap,
    VkBuffer treeVertexBuffer, VkBuffer treeIndexBuffer, const std::vector<TreeSubmeshRenderInfo>& submeshes,
    VkImageView defaultTextureView, VkSampler defaultTextureSampler) {
    m_MaxChunks = maxChunks;
    m_TreeVertexBuffer = treeVertexBuffer;
    m_TreeIndexBuffer = treeIndexBuffer;
    m_SubmeshRenderInfo = submeshes;   // CHANGED — store the submesh list, no more single treeIndexCount
    m_DefaultTextureView = defaultTextureView;
    m_DefaultTextureSampler = defaultTextureSampler;
    m_Pool.Init(m_Engine, maxTreesPerChunk, maxChunks);

    std::vector<TreeSubmeshInfo> cullerSubmeshInfo;   // CHANGED — build the culler-facing submesh list
    for (auto& s : submeshes) cullerSubmeshInfo.push_back({ s.indexCount, s.firstIndex });
    m_Culler.Init(m_Engine, &m_Pool, maxChunks, cullerSubmeshInfo);

    CreateGraphicsPipeline();

    VkDeviceSize lightingBufferSize = sizeof(LightingUBO);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_Engine->CreateBuffer(lightingBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_LightingUBOBuffers[i], m_LightingUBOMemory[i]);
        vkMapMemory(m_Engine->GetDevice(), m_LightingUBOMemory[i], 0, lightingBufferSize, 0, &m_LightingUBOMapped[i]);
    }

    VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT] = { m_LightingDescriptorSetLayout, m_LightingDescriptorSetLayout };
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_LightingDescriptorPool;
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts;
    vkAllocateDescriptorSets(m_Engine->GetDevice(), &allocInfo, m_LightingDescriptorSets);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{ m_LightingUBOBuffers[i], 0, sizeof(LightingUBO) };
        VkDescriptorImageInfo shadowImageInfo{};
        shadowImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        shadowImageInfo.imageView = shadowMap->GetImageView();
        shadowImageInfo.sampler = shadowMap->GetSampler();

        VkWriteDescriptorSet writes[2]{};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = m_LightingDescriptorSets[i];
        writes[0].dstBinding = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].descriptorCount = 1;
        writes[0].pBufferInfo = &bufferInfo;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = m_LightingDescriptorSets[i];
        writes[1].dstBinding = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].descriptorCount = 1;
        writes[1].pImageInfo = &shadowImageInfo;

        vkUpdateDescriptorSets(m_Engine->GetDevice(), 2, writes, 0, nullptr);
    }

    std::cout << "TreeRenderer initialized" << std::endl;
}

std::optional<uint32_t> TreeRenderer::AllocateChunkSlot() { return m_Pool.AllocateSlot(); }
void TreeRenderer::FreeChunkSlot(uint32_t slot) { m_Pool.ClearMetadata(slot); m_Pool.FreeSlot(slot); }
void TreeRenderer::ProcessPendingFrees() { m_Pool.ProcessPendingFrees(); }
void TreeRenderer::UploadChunkBounds(uint32_t slot, glm::vec3 boundsCenter, float boundsRadius) {
    // instanceCount is set by the generation shader directly — this just refreshes bounds/isActive
    m_Pool.UploadBoundsOnly(slot, boundsCenter, boundsRadius);   // NEW small helper — see note below
}

void TreeRenderer::RecordCullingPass(VkCommandBuffer commandBuffer, const Frustum& frustum) {
    m_Culler.RecordCullingCommands(commandBuffer, frustum);
    m_Culler.RecordBarrier(commandBuffer);
}

void TreeRenderer::RecordDraw(VkCommandBuffer commandBuffer, const Camera3D& camera, DayNightCycle& dayNightCycle,
    const glm::mat4& lightSpaceMatrix, const std::vector<PointLight>& activeLights) {
    uint32_t frameIndex = m_Engine->GetCurrentFrameIndex();

    VkExtent2D extent = m_Engine->GetSwapChainExtent();
    float aspect = static_cast<float>(extent.width) / extent.height;
    glm::mat4 view = camera.GetActiveViewMatrix();
    glm::mat4 proj = camera.GetProjectionMatrix(aspect);

    LightingUBO lighting{};
    lighting.viewProj = proj * view;
    lighting.sunDirection = glm::vec4(dayNightCycle.GetSunDirection(), 0.0f);
    lighting.sunColor = glm::vec4(dayNightCycle.GetSunColor(), dayNightCycle.GetAmbientIntensity());
    lighting.lightSpaceMatrix = lightSpaceMatrix;
    lighting.numPointLights = static_cast<int>(std::min(activeLights.size(), (size_t)MAX_POINT_LIGHTS));
    for (int i = 0; i < lighting.numPointLights; i++) lighting.pointLights[i] = activeLights[i];
    memcpy(m_LightingUBOMapped[frameIndex], &lighting, sizeof(LightingUBO));

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout,
        0, 1, &m_LightingDescriptorSets[frameIndex], 0, nullptr);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout,
        1, 1, &m_InstanceDescSet, 0, nullptr);   // ADD — was missing entirely


    VkViewport viewport{ 0, 0, (float)extent.width, (float)extent.height, 0, 1 };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    VkRect2D scissor{ {0,0}, extent };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = { m_TreeVertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_TreeIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
    for (uint32_t s = 0; s < m_Culler.GetSubmeshCount(); s++) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout,
            2, 1, &m_SubmeshTextureDescSets[s], 0, nullptr);

        vkCmdDrawIndexedIndirectCount(commandBuffer, m_Culler.GetDrawCommandBuffer(s), 0,
            m_Culler.GetDrawCountBuffer(s), 0, m_MaxChunks, sizeof(GpuDrawCommand));
    }
}

void TreeRenderer::CreateGraphicsPipeline() {
    VkDevice device = m_Engine->GetDevice();

    // ===== Set 0: lighting UBO + shadow sampler =====
    VkDescriptorSetLayoutBinding lightingBindings[2]{};
    lightingBindings[0].binding = 0;
    lightingBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightingBindings[0].descriptorCount = 1;
    lightingBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    lightingBindings[1].binding = 1;
    lightingBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    lightingBindings[1].descriptorCount = 1;
    lightingBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo lightingLayoutInfo{};
    lightingLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    lightingLayoutInfo.bindingCount = 2;
    lightingLayoutInfo.pBindings = lightingBindings;
    vkCreateDescriptorSetLayout(device, &lightingLayoutInfo, nullptr, &m_LightingDescriptorSetLayout);

    VkDescriptorPoolSize lightingPoolSizes[2]{
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_FRAMES_IN_FLIGHT }
    };
    VkDescriptorPoolCreateInfo lightingPoolInfo{};
    lightingPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    lightingPoolInfo.poolSizeCount = 2;
    lightingPoolInfo.pPoolSizes = lightingPoolSizes;
    lightingPoolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
    vkCreateDescriptorPool(device, &lightingPoolInfo, nullptr, &m_LightingDescriptorPool);

    // ===== Set 1: instance buffer =====
    VkDescriptorSetLayoutBinding instanceBinding{};
    instanceBinding.binding = 0;
    instanceBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    instanceBinding.descriptorCount = 1;
    instanceBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo instanceLayoutInfo{};
    instanceLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    instanceLayoutInfo.bindingCount = 1;
    instanceLayoutInfo.pBindings = &instanceBinding;
    vkCreateDescriptorSetLayout(device, &instanceLayoutInfo, nullptr, &m_InstanceDescSetLayout);

    VkDescriptorPoolSize instancePoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 };
    VkDescriptorPoolCreateInfo instancePoolInfo{};
    instancePoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    instancePoolInfo.poolSizeCount = 1;
    instancePoolInfo.pPoolSizes = &instancePoolSize;
    instancePoolInfo.maxSets = 1;
    vkCreateDescriptorPool(device, &instancePoolInfo, nullptr, &m_InstanceDescPool);

    VkDescriptorSetAllocateInfo instanceAllocInfo{};
    instanceAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    instanceAllocInfo.descriptorPool = m_InstanceDescPool;
    instanceAllocInfo.descriptorSetCount = 1;
    instanceAllocInfo.pSetLayouts = &m_InstanceDescSetLayout;
    vkAllocateDescriptorSets(device, &instanceAllocInfo, &m_InstanceDescSet);

    VkDescriptorBufferInfo instanceBufferInfo{ m_Pool.GetInstanceBuffer(), 0, VK_WHOLE_SIZE };
    VkWriteDescriptorSet instanceWrite{};
    instanceWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    instanceWrite.dstSet = m_InstanceDescSet;
    instanceWrite.dstBinding = 0;
    instanceWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    instanceWrite.descriptorCount = 1;
    instanceWrite.pBufferInfo = &instanceBufferInfo;
    vkUpdateDescriptorSets(device, 1, &instanceWrite, 0, nullptr);

    // ===== Set 2: per-submesh texture =====
    VkDescriptorSetLayoutBinding texBinding{};
    texBinding.binding = 0;
    texBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texBinding.descriptorCount = 1;
    texBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo texLayoutInfo{};
    texLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    texLayoutInfo.bindingCount = 1;
    texLayoutInfo.pBindings = &texBinding;
    vkCreateDescriptorSetLayout(device, &texLayoutInfo, nullptr, &m_TextureDescSetLayout);

    VkDescriptorPoolSize texPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(m_SubmeshRenderInfo.size()) };
    VkDescriptorPoolCreateInfo texPoolInfo{};
    texPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    texPoolInfo.poolSizeCount = 1;
    texPoolInfo.pPoolSizes = &texPoolSize;
    texPoolInfo.maxSets = static_cast<uint32_t>(m_SubmeshRenderInfo.size());
    vkCreateDescriptorPool(device, &texPoolInfo, nullptr, &m_TextureDescPool);

    m_SubmeshTextureDescSets.resize(m_SubmeshRenderInfo.size());
    for (size_t i = 0; i < m_SubmeshRenderInfo.size(); i++) {
        VkDescriptorSetAllocateInfo texAllocInfo{};
        texAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        texAllocInfo.descriptorPool = m_TextureDescPool;
        texAllocInfo.descriptorSetCount = 1;
        texAllocInfo.pSetLayouts = &m_TextureDescSetLayout;
        vkAllocateDescriptorSets(device, &texAllocInfo, &m_SubmeshTextureDescSets[i]);

        VkImageView viewToUse = m_SubmeshRenderInfo[i].textureView != VK_NULL_HANDLE
            ? m_SubmeshRenderInfo[i].textureView : m_DefaultTextureView;   // FALLBACK
        VkSampler samplerToUse = m_SubmeshRenderInfo[i].textureSampler != VK_NULL_HANDLE
            ? m_SubmeshRenderInfo[i].textureSampler : m_DefaultTextureSampler;   // FALLBACK

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = viewToUse;
        imageInfo.sampler = samplerToUse;

        VkWriteDescriptorSet texWrite{};
        texWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        texWrite.dstSet = m_SubmeshTextureDescSets[i];
        texWrite.dstBinding = 0;
        texWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        texWrite.descriptorCount = 1;
        texWrite.pImageInfo = &imageInfo;
        vkUpdateDescriptorSets(device, 1, &texWrite, 0, nullptr);
    }

    // ===== Pipeline layout: all 3 sets =====
    VkDescriptorSetLayout setLayouts[3] = { m_LightingDescriptorSetLayout, m_InstanceDescSetLayout, m_TextureDescSetLayout };
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 3;
    pipelineLayoutInfo.pSetLayouts = setLayouts;
    vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout);

    auto vertCode = m_Engine->ReadFile("Shaders/tree_instanced_vert.spv");
    auto fragCode = m_Engine->ReadFile("Shaders/tree_instanced_frag.spv");
    VkShaderModule vertModule = m_Engine->CreateShaderModule(vertCode);
    VkShaderModule fragModule = m_Engine->CreateShaderModule(fragCode);

    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName = "main";

    VkPipelineShaderStageCreateInfo stages[] = { vertStage, fragStage };

    auto bindingDesc = Vertex::getBindingDescription();
    auto attrDescs = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescs.size());
    vertexInputInfo.pVertexAttributeDescriptions = attrDescs.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    VkFormat colorFormat = m_Engine->GetSwapChainFormat();
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &colorFormat;
    renderingInfo.depthAttachmentFormat = m_Engine->GetDepthFormat();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_PipelineLayout;
    pipelineInfo.renderPass = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create tree pipeline");
    }

    vkDestroyShaderModule(device, fragModule, nullptr);
    vkDestroyShaderModule(device, vertModule, nullptr);
}