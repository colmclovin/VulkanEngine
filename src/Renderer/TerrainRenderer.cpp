// TerrainRenderer.cpp
#include "../Renderer/TerrainRenderer.h"
#include "../Engine/VulkanEngine.h"   // adjust path to match your actual project structure
#include "../Rendering/LightingUBO.h"
#include <iostream>
#include "../Rendering/Mesh.h"
#include "../Rendering/Vertex.h"
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>
#include <glm/glm.hpp>
#include "../Components/Components.h"
#include "../Rendering/Frustum.h"
#include <unordered_map>
#include "../Rendering/GpuDrawCommand.h"

TerrainRenderer::TerrainRenderer(VulkanEngine* engine) : m_Engine(engine) {}

void TerrainRenderer::Init(uint32_t vertsPerChunk, uint32_t indicesPerChunk, uint32_t maxChunks, ShadowMap* shadowMap) {
    m_VertsPerChunk = vertsPerChunk;
    m_IndicesPerChunk = indicesPerChunk;
    m_MaxChunks = maxChunks;

    m_Pool.Init(m_Engine, vertsPerChunk, indicesPerChunk, maxChunks);
    m_Culler.Init(m_Engine, &m_Pool, maxChunks, indicesPerChunk, vertsPerChunk);

    CreateGraphicsPipeline();

    // Lighting UBO — same pattern as MeshRenderer's, terrain needs the same sun/shadow data
    VkDeviceSize lightingBufferSize = sizeof(LightingUBO);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_Engine->CreateBuffer(lightingBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_LightingUBOBuffers[i], m_LightingUBOMemory[i]);
        vkMapMemory(m_Engine->GetDevice(), m_LightingUBOMemory[i], 0, lightingBufferSize, 0, &m_LightingUBOMapped[i]);
    }

    // Allocate + write lighting descriptor sets (UBO only — terrain doesn't need a texture sampler yet, it's vertex-colored)
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

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;   // NEW
        writes[1].dstSet = m_LightingDescriptorSets[i];
        writes[1].dstBinding = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].descriptorCount = 1;
        writes[1].pImageInfo = &shadowImageInfo;

        vkUpdateDescriptorSets(m_Engine->GetDevice(), 2, writes, 0, nullptr);   // CHANGED — 2 writes now
    }
}

std::optional<uint32_t> TerrainRenderer::AllocateChunkSlot() {
    return m_Pool.AllocateSlot();
}

void TerrainRenderer::UploadChunk(uint32_t slot, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices,
    glm::vec3 boundsCenter, float boundsRadius) {
    m_Pool.UploadChunkData(slot, vertices, indices);
    m_Pool.UploadMetadata(slot, boundsCenter, boundsRadius);
}

void TerrainRenderer::FreeChunkSlot(uint32_t slot) {
    m_Pool.ClearMetadata(slot);
    m_Pool.FreeSlot(slot);
}

void TerrainRenderer::CreateGraphicsPipeline() {
    // Lighting descriptor set layout: just the UBO, binding 0
    VkDescriptorSetLayoutBinding bindings[2]{};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[1].binding = 1;   // NEW — shadow map sampler
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;   // CHANGED — was 1
    layoutInfo.pBindings = bindings;
    vkCreateDescriptorSetLayout(m_Engine->GetDevice(), &layoutInfo, nullptr, &m_LightingDescriptorSetLayout);

    VkDescriptorPoolSize poolSizes[2]{};
    poolSizes[0] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT };
    poolSizes[1] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_FRAMES_IN_FLIGHT };   // NEW

  
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
    vkCreateDescriptorPool(m_Engine->GetDevice(), &poolInfo, nullptr, &m_LightingDescriptorPool);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_LightingDescriptorSetLayout;
    vkCreatePipelineLayout(m_Engine->GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout);
    // NOTE: no push constants needed — terrain vertices already have absolute world position baked in,
    // and lighting comes entirely from the UBO. gl_Position = viewProj * vec4(inPosition, 1.0) directly.

    auto vertCode = m_Engine->ReadFile("Shaders/terrain_vert.spv");
    auto fragCode = m_Engine->ReadFile("Shaders/terrain_frag.spv");
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

    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    if (vkCreateGraphicsPipelines(m_Engine->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create terrain pipeline");
    }
    rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    if (vkCreateGraphicsPipelines(m_Engine->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_WireframePipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create terrain wireframe pipeline");
    }

    vkDestroyShaderModule(m_Engine->GetDevice(), fragModule, nullptr);
    vkDestroyShaderModule(m_Engine->GetDevice(), vertModule, nullptr);
}

void TerrainRenderer::Render(VkCommandBuffer commandBuffer, const Camera3D& camera, DayNightCycle& dayNightCycle,
    const glm::mat4& lightSpaceMatrix, const std::vector<PointLight>& activeLights, bool wireframe) {
    uint32_t frameIndex = m_Engine->GetCurrentFrameIndex();

    VkExtent2D extent = m_Engine->GetSwapChainExtent();
    float aspect = static_cast<float>(extent.width) / extent.height;
    glm::mat4 view = camera.GetActiveViewMatrix();
    glm::mat4 proj = camera.GetProjectionMatrix(aspect);
    Frustum frustum = Frustum::FromViewProj(proj * view);

    // --- Compute pass: record into the SAME command buffer, no separate submission/wait ---
    m_Culler.RecordCullingCommands(commandBuffer, frustum);
    m_Culler.RecordBarrier(commandBuffer);

    // --- Write lighting UBO (unchanged, host-visible/coherent, safe to write directly from CPU) ---
    LightingUBO lighting{};
    lighting.viewProj = proj * view;
    lighting.sunDirection = glm::vec4(dayNightCycle.GetSunDirection(), 0.0f);
    lighting.sunColor = glm::vec4(dayNightCycle.GetSunColor(), dayNightCycle.GetAmbientIntensity());
    lighting.lightSpaceMatrix = lightSpaceMatrix;
    lighting.numPointLights = static_cast<int>(std::min(activeLights.size(), (size_t)MAX_POINT_LIGHTS));
    for (int i = 0; i < lighting.numPointLights; i++) lighting.pointLights[i] = activeLights[i];
    memcpy(m_LightingUBOMapped[frameIndex], &lighting, sizeof(LightingUBO));

    // --- Graphics pass: one indirect draw, count read entirely on the GPU ---
    VkPipeline pipelineToUse = wireframe ? m_WireframePipeline : m_Pipeline;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineToUse);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout,
        0, 1, &m_LightingDescriptorSets[frameIndex], 0, nullptr);

    VkViewport viewport{ 0, 0, (float)extent.width, (float)extent.height, 0, 1 };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    VkRect2D scissor{ {0,0}, extent };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = { m_Pool.GetVertexBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_Pool.GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexedIndirectCount(commandBuffer, m_Culler.GetDrawCommandBuffer(), 0,
        m_Culler.GetDrawCountBuffer(), 0, m_MaxChunks, sizeof(GpuDrawCommand));
}

void TerrainRenderer::RecordCullingPass(VkCommandBuffer commandBuffer, const Frustum& frustum) {
    m_Culler.RecordCullingCommands(commandBuffer, frustum);
    m_Culler.RecordBarrier(commandBuffer);
}

void TerrainRenderer::RecordDraw(VkCommandBuffer commandBuffer, const Camera3D& camera, DayNightCycle& dayNightCycle,
    const glm::mat4& lightSpaceMatrix, const std::vector<PointLight>& activeLights, bool wireframe) {
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

    VkPipeline pipelineToUse = wireframe ? m_WireframePipeline : m_Pipeline;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineToUse);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout,
        0, 1, &m_LightingDescriptorSets[frameIndex], 0, nullptr);

    VkViewport viewport{ 0, 0, (float)extent.width, (float)extent.height, 0, 1 };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    VkRect2D scissor{ {0,0}, extent };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = { m_Pool.GetVertexBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_Pool.GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexedIndirectCount(commandBuffer, m_Culler.GetDrawCommandBuffer(), 0,
        m_Culler.GetDrawCountBuffer(), 0, m_MaxChunks, sizeof(GpuDrawCommand));
    //std::cout << "Terrain indirect draw issued, maxChunks=" << m_MaxChunks << std::endl;
}
void TerrainRenderer::UploadChunkBounds(uint32_t slot, glm::vec3 boundsCenter, float boundsRadius) {
    m_Pool.UploadMetadata(slot, boundsCenter, boundsRadius);
}