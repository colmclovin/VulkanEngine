// ShadowMapRenderer.cpp
#include "ShadowMapRenderer.h"
#include "../Engine/VulkanEngine.h"
#include "ShadowMap.h"
#include "../Components/Components.h"
#include "../Rendering/Frustum.h"
#include <stdexcept>
#include <iostream>
ShadowMapRenderer::ShadowMapRenderer(VulkanEngine* engine) : m_Engine(engine) {}

void ShadowMapRenderer::Init() {
    CreatePipeline();
    CreateInstancedPipeline();   // NEW — is this line actually present?
}

void ShadowMapRenderer::CreatePipeline() {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;
    vkCreatePipelineLayout(m_Engine->GetDevice(), &layoutInfo, nullptr, &m_PipelineLayout);

    auto vertCode = m_Engine->ReadFile("Shaders/shadow_vert.spv");
    auto fragCode = m_Engine->ReadFile("Shaders/shadow_frag.spv");
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

    // Only need position — reuse Vertex's binding but restrict to 1 attribute for this pipeline
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(Vertex);   // must match your real Vertex stride even though we only read position
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription positionAttr{};
    positionAttr.binding = 0;
    positionAttr.location = 0;
    positionAttr.format = VK_FORMAT_R32G32B32_SFLOAT;
    positionAttr.offset = offsetof(Vertex, position);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &binding;
    vertexInputInfo.vertexAttributeDescriptionCount = 1;
    vertexInputInfo.pVertexAttributeDescriptions = &positionAttr;

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
    rasterizer.cullMode = VK_CULL_MODE_NONE; // front-face culling for shadow pass reduces peter-panning/acne — common trick
    rasterizer.depthBiasEnable = VK_TRUE;            // we'll tune bias constants once we see artifacts

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 0;   // no color attachment at all for a depth-only pass

    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 0;               // no color output
    renderingInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;

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
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_PipelineLayout;
    pipelineInfo.renderPass = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(m_Engine->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shadow map pipeline");
    }

    vkDestroyShaderModule(m_Engine->GetDevice(), fragModule, nullptr);
    vkDestroyShaderModule(m_Engine->GetDevice(), vertModule, nullptr);
}

// ShadowMapRenderer.cpp
void ShadowMapRenderer::BeginShadowPass(ShadowMap &shadowMap) {
    VkCommandBuffer commandBuffer = m_Engine->GetCurrentCommandBuffer();

    VkImageMemoryBarrier toDepth{};
    toDepth.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toDepth.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    toDepth.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    toDepth.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toDepth.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toDepth.image = shadowMap.GetImage();
    toDepth.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
    toDepth.srcAccessMask = 0;
    toDepth.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &toDepth);

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = shadowMap.GetImageView();
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.depthStencil = { 1.0f, 0 };

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = { { 0, 0 }, { shadowMap.GetResolution(), shadowMap.GetResolution() } };
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 0;
    renderingInfo.pDepthAttachment = &depthAttachment;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    VkViewport viewport{ 0, 0, (float)shadowMap.GetResolution(), (float)shadowMap.GetResolution(), 0, 1 };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    VkRect2D scissor{ { 0, 0 }, { shadowMap.GetResolution(), shadowMap.GetResolution() } };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void ShadowMapRenderer::RenderStatic(entt::registry& registry, const glm::mat4& lightSpaceMatrix) {
    VkCommandBuffer commandBuffer = m_Engine->GetCurrentCommandBuffer();
    Frustum lightFrustum = Frustum::FromViewProj(lightSpaceMatrix);

    std::unordered_map<Mesh*, std::vector<entt::entity>> meshGroups;
    auto view = registry.view<TransformComponent, MeshComponent>();
    for (auto entity : view) {
        auto& transform = view.get<TransformComponent>(entity);
        auto& meshComp = view.get<MeshComponent>(entity);
        if (!meshComp.mesh || !meshComp.mesh->IsUploaded()) continue;

        glm::vec3 cullCenter = transform.Position;
        float cullRadius = 5.0f;
        if (registry.any_of<ChunkBoundsComponent>(entity)) {
            auto& cb = registry.get<ChunkBoundsComponent>(entity);
            cullCenter = cb.center;
            cullRadius = cb.radius;
        }
        else if (registry.any_of<BoundsComponent>(entity)) {
            auto& bounds = registry.get<BoundsComponent>(entity);
            cullRadius = glm::length(bounds.halfExtents);
        }
        if (!lightFrustum.ContainsSphere(cullCenter, cullRadius)) continue;

        meshGroups[meshComp.mesh.get()].push_back(entity);
    }

    const size_t INSTANCING_THRESHOLD = 4;

    size_t totalInstancedCount = 0;
    for (auto& [meshPtr, entities] : meshGroups) {
        if (entities.size() >= INSTANCING_THRESHOLD) totalInstancedCount += entities.size();
    }
    if (totalInstancedCount > 0) {
        EnsureInstanceBufferCapacity(totalInstancedCount);
    }
    m_InstanceBufferWriteOffset = 0;

    for (auto& [meshPtr, entities] : meshGroups) {
        if (entities.size() >= INSTANCING_THRESHOLD) {
            DrawInstancedShadowGroup(registry, meshPtr, entities, commandBuffer, lightSpaceMatrix);
        }
        else {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
            for (auto entity : entities) {
                auto& transform = registry.get<TransformComponent>(entity);
                auto& meshComp = registry.get<MeshComponent>(entity);

                glm::mat4 lightSpaceMVP = lightSpaceMatrix * transform.GetMatrix();
                vkCmdPushConstants(commandBuffer, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &lightSpaceMVP);

                VkBuffer vertexBuffers[] = { meshComp.mesh->vertexBuffer };
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
                vkCmdBindIndexBuffer(commandBuffer, meshComp.mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(meshComp.mesh->Indices.size()), 1, 0, 0, 0);
            }
        }
    }
}

void ShadowMapRenderer::EndShadowPass(ShadowMap &shadowMap) {
    VkCommandBuffer commandBuffer = m_Engine->GetCurrentCommandBuffer();
    vkCmdEndRendering(commandBuffer);

    VkImageMemoryBarrier toShaderRead{};
    toShaderRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toShaderRead.oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    toShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    toShaderRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toShaderRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toShaderRead.image = shadowMap.GetImage();
    toShaderRead.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
    toShaderRead.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    toShaderRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &toShaderRead);
}

void ShadowMapRenderer::Shutdown() {
    VkDevice device = m_Engine->GetDevice();

    if (m_Pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_Pipeline, nullptr);
    }
    if (m_PipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
    }

    // NEW — instanced pipeline
    if (m_InstancedPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_InstancedPipeline, nullptr);
    }
    if (m_InstancedPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_InstancedPipelineLayout, nullptr);
    }

    // NEW — instance buffer
    if (m_InstanceBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_InstanceBuffer, nullptr);
        vkFreeMemory(device, m_InstanceBufferMemory, nullptr);
    }
}

void ShadowMapRenderer::CreateInstancedPipeline() {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4);   // just lightSpaceMatrix now — model comes from instance buffer

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    vkCreatePipelineLayout(m_Engine->GetDevice(), &pipelineLayoutInfo, nullptr, &m_InstancedPipelineLayout);

    auto vertCode = m_Engine->ReadFile("Shaders/shadow_instanced_vert.spv");
    auto fragCode = m_Engine->ReadFile("Shaders/shadow_frag.spv");   // reuse the existing empty depth-only fragment shader
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

    // Binding 0: mesh position only (matches shadow.vert's original single-attribute layout)
    VkVertexInputBindingDescription meshBinding{};
    meshBinding.binding = 0;
    meshBinding.stride = sizeof(Vertex);
    meshBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription positionAttr{};
    positionAttr.binding = 0;
    positionAttr.location = 0;
    positionAttr.format = VK_FORMAT_R32G32B32_SFLOAT;
    positionAttr.offset = offsetof(Vertex, position);

    auto instanceBinding = GetInstanceBindingDescription();
    auto instanceAttrs = GetInstanceAttributeDescriptions();

    VkVertexInputBindingDescription bindings[] = { meshBinding, instanceBinding };

    std::vector<VkVertexInputAttributeDescription> allAttrs;
    allAttrs.push_back(positionAttr);
    for (auto& a : instanceAttrs) allAttrs.push_back(a);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 2;
    vertexInputInfo.pVertexBindingDescriptions = bindings;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(allAttrs.size());
    vertexInputInfo.pVertexAttributeDescriptions = allAttrs.data();

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
    rasterizer.cullMode = VK_CULL_MODE_NONE;   // matches your earlier culling diagnostic change — adjust if you settled on FRONT_BIT again
    rasterizer.depthBiasEnable = VK_TRUE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 0;

    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 0;
    renderingInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;

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
    pipelineInfo.layout = m_InstancedPipelineLayout;
    pipelineInfo.renderPass = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(m_Engine->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_InstancedPipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create instanced shadow pipeline");
    }

    vkDestroyShaderModule(m_Engine->GetDevice(), fragModule, nullptr);
    vkDestroyShaderModule(m_Engine->GetDevice(), vertModule, nullptr);
}
void ShadowMapRenderer::EnsureInstanceBufferCapacity(size_t instanceCount) {
    if (instanceCount <= m_InstanceBufferCapacity) return;

    VkDevice device = m_Engine->GetDevice();
    if (m_InstanceBuffer != VK_NULL_HANDLE) {
        m_Engine->QueueBufferDestruction(m_InstanceBuffer, m_InstanceBufferMemory);
    }

    size_t newCapacity = std::max(instanceCount, m_InstanceBufferCapacity * 2);
    VkDeviceSize bufferSize = newCapacity * sizeof(ShadowInstanceData);

    m_Engine->CreateBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_InstanceBuffer, m_InstanceBufferMemory);
    vkMapMemory(device, m_InstanceBufferMemory, 0, bufferSize, 0, &m_InstanceBufferMapped);

    m_InstanceBufferCapacity = newCapacity;
}
void ShadowMapRenderer::DrawInstancedShadowGroup(entt::registry& registry, Mesh* mesh, const std::vector<entt::entity>& entities,
    VkCommandBuffer commandBuffer, const glm::mat4& lightSpaceMatrix) {
    std::vector<ShadowInstanceData> instances;
    instances.reserve(entities.size());
    for (auto entity : entities) {
        auto& transform = registry.get<TransformComponent>(entity);
        instances.push_back({ transform.GetMatrix() });
    }

    size_t writeOffsetBytes = m_InstanceBufferWriteOffset * sizeof(ShadowInstanceData);
    memcpy(static_cast<char*>(m_InstanceBufferMapped) + writeOffsetBytes,
        instances.data(), instances.size() * sizeof(ShadowInstanceData));

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_InstancedPipeline);
    vkCmdPushConstants(commandBuffer, m_InstancedPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &lightSpaceMatrix);

    VkBuffer vertexBuffers[] = { mesh->vertexBuffer, m_InstanceBuffer };
    VkDeviceSize offsets[] = { 0, writeOffsetBytes };
    vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh->Indices.size()), static_cast<uint32_t>(instances.size()), 0, 0, 0);

    m_InstanceBufferWriteOffset += instances.size();
}

VkVertexInputBindingDescription ShadowMapRenderer::GetInstanceBindingDescription() {
    VkVertexInputBindingDescription binding{};
    binding.binding = 1;
    binding.stride = sizeof(ShadowInstanceData);
    binding.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
    return binding;
}

std::array<VkVertexInputAttributeDescription, 4> ShadowMapRenderer::GetInstanceAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 4> attrs{};
    attrs[0] = { 1, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(ShadowInstanceData, model) + 0 * sizeof(glm::vec4) };
    attrs[1] = { 2, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(ShadowInstanceData, model) + 1 * sizeof(glm::vec4) };
    attrs[2] = { 3, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(ShadowInstanceData, model) + 2 * sizeof(glm::vec4) };
    attrs[3] = { 4, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(ShadowInstanceData, model) + 3 * sizeof(glm::vec4) };
    return attrs;
}