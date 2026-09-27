#include "MeshRenderer.h"
#include <iostream>
#include "../Rendering/Mesh.h"
#include "../Rendering/Vertex.h"
#include "../Engine/VulkanEngine.h"
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>
#include <glm/glm.hpp>
#include "../Components/Components.h"
#include "../Rendering/LightingUBO.h"
#include <unordered_map>

MeshRenderer::MeshRenderer(VulkanEngine* engine) : m_Engine(engine) {
}

MeshRenderer::~MeshRenderer() {
    Shutdown();
}

void MeshRenderer::Init(ShadowMap* shadowMap) {
    std::cout << "Initializing Mesh Renderer..." << std::endl;
    m_ShadowMap = shadowMap;



    CreatePipeline();
    CreateInstancedPipeline();
    CreateUniformBuffers();



    VkDescriptorSetLayout lightingLayouts[MAX_FRAMES_IN_FLIGHT];
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) lightingLayouts[i] = m_LightingDescriptorSetLayout;

    VkDescriptorSetAllocateInfo lightingAllocInfo{};
    lightingAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    lightingAllocInfo.descriptorPool = m_LightingDescriptorPool;
    lightingAllocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    lightingAllocInfo.pSetLayouts = lightingLayouts;
    vkAllocateDescriptorSets(m_Engine->GetDevice(), &lightingAllocInfo, m_LightingDescriptorSets);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo lightingBufferInfo{};
        lightingBufferInfo.buffer = m_LightingUBOBuffers[i];
        lightingBufferInfo.offset = 0;
        lightingBufferInfo.range = sizeof(LightingUBO);

        VkDescriptorImageInfo shadowMapInfo{};                              // NEW
        shadowMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        shadowMapInfo.imageView = shadowMap->GetImageView();
        shadowMapInfo.sampler = shadowMap->GetSampler();

        VkWriteDescriptorSet writes[2]{};                                    // CHANGED — was a single write
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = m_LightingDescriptorSets[i];
        writes[0].dstBinding = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].descriptorCount = 1;
        writes[0].pBufferInfo = &lightingBufferInfo;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;            // NEW
        writes[1].dstSet = m_LightingDescriptorSets[i];
        writes[1].dstBinding = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].descriptorCount = 1;
        writes[1].pImageInfo = &shadowMapInfo;

        vkUpdateDescriptorSets(m_Engine->GetDevice(), 2, writes, 0, nullptr);   // CHANGED — count 2, was 1
    }




    TextureData whitePixel;
    whitePixel.width = 1;
    whitePixel.height = 1;
    whitePixel.pixels = { 255, 255, 255, 255 };

    m_DefaultTexture = std::make_shared<Texture>();
    m_DefaultTexture->UploadToGPU(m_Engine, whitePixel);

    m_DefaultDescriptorSet = AllocateTextureDescriptorSet(m_DefaultTexture->imageView, m_DefaultTexture->sampler);




    m_initialized = true;
    std::cout << "Mesh Renderer initialized" << std::endl;
}

void MeshRenderer::Render(entt::registry &registry, const Camera3D &camera, bool wireframe, DayNightCycle &dayNightCycle, const glm::mat4 &lightSpaceMatrix, const std::vector<PointLight> &activeLights) {
    VkCommandBuffer commandBuffer = m_Engine->GetCurrentCommandBuffer();
    uint32_t frameIndex = m_Engine->GetCurrentFrameIndex(); // ADD THIS
    m_InstanceBufferWriteOffset = 0;

    VkPipeline pipelineToUse = wireframe ? m_WireframePipeline : m_Pipeline;
    if (pipelineToUse != VK_NULL_HANDLE) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineToUse);
    }

    VkExtent2D extent = m_Engine->GetSwapChainExtent();
    float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);

    VkViewport viewport{ 0.0f, 0.0f, (float)extent.width, (float)extent.height, 0.0f, 1.0f };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    VkRect2D scissor{ { 0, 0 }, extent };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    glm::mat4 view = camera.GetActiveViewMatrix();
    glm::mat4 proj = camera.GetProjectionMatrix(aspect);

    LightingUBO lighting{};
    lighting.viewProj = proj * view; // NEW
    lighting.sunDirection = glm::vec4(dayNightCycle.GetSunDirection(), 0.0f);
    lighting.sunColor = glm::vec4(dayNightCycle.GetSunColor(), dayNightCycle.GetAmbientIntensity());
    lighting.lightSpaceMatrix = lightSpaceMatrix;
    lighting.numPointLights = static_cast<int>(activeLights.size());
    for (size_t i = 0; i < activeLights.size(); i++) {
        lighting.pointLights[i] = activeLights[i];
    }
    memcpy(m_LightingUBOMapped[frameIndex], &lighting, sizeof(LightingUBO));

    // ADD — bind the lighting set once per frame too (set index 1); texture set (index 0) still bound per-draw below
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout,
                            1, 1, &m_LightingDescriptorSets[frameIndex], 0, nullptr);

    std::unordered_map<Mesh *, std::vector<entt::entity>> meshGroups;
    auto view3D = registry.view<TransformComponent, MeshComponent>();
    for (auto entity : view3D) {
        auto &meshComp = view3D.get<MeshComponent>(entity);
        if (!meshComp.mesh) continue;
        meshComp.mesh->UploadToGPU(m_Engine);
        meshGroups[meshComp.mesh.get()].push_back(entity);
    }

 size_t totalInstancedCount = 0;
    for (auto &[meshPtr, entities] : meshGroups) {
        if (entities.size() >= INSTANCING_THRESHOLD) totalInstancedCount += entities.size();
    }
    if (totalInstancedCount > 0) {
        EnsureInstanceBufferCapacity(totalInstancedCount);
    }
    m_InstanceBufferWriteOffset = 0;

    for (auto &[meshPtr, entities] : meshGroups) {
        if (entities.size() >= INSTANCING_THRESHOLD) {
            DrawInstancedGroup(registry, meshPtr, entities, commandBuffer);
        } else {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineToUse);
            for (auto entity : entities) {
                DrawSingleEntity(registry, entity, commandBuffer, view, proj);
            }
        }
    }
}

void MeshRenderer::DrawSingleEntity(entt::registry &registry, entt::entity entity, VkCommandBuffer commandBuffer,
                                    const glm::mat4 &view, const glm::mat4 &proj) {
    auto &transform = registry.get<TransformComponent>(entity);
    auto &meshComp = registry.get<MeshComponent>(entity);

    if (!meshComp.mesh) return;
    meshComp.mesh->UploadToGPU(m_Engine);

    VkBuffer vertexBuffers[] = { meshComp.mesh->vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, meshComp.mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout,
                            1, 1, &m_LightingDescriptorSets[m_Engine->GetCurrentFrameIndex()], 0, nullptr);


    bool isGhost = registry.any_of<GhostComponent>(entity);
    bool isBlockedGhost = isGhost && registry.get<GhostComponent>(entity).blocked;

    glm::vec4 baseTint = glm::vec4(1.0f);
    if (registry.any_of<BeltComponent>(entity)) {
        auto &belt = registry.get<BeltComponent>(entity);
        if (belt.direction.x > 0.5f)
            baseTint = glm::vec4(0.2f, 0.6f, 1.0f, 1.0f);
        else if (belt.direction.x < -0.5f)
            baseTint = glm::vec4(1.0f, 0.6f, 0.2f, 1.0f);
        else if (belt.direction.z > 0.5f)
            baseTint = glm::vec4(0.2f, 1.0f, 0.4f, 1.0f);
        else
            baseTint = glm::vec4(1.0f, 0.3f, 0.3f, 1.0f);
    }

    if (meshComp.mesh->SubMeshes.empty()) {
        glm::mat4 model = transform.GetMatrix();

        MeshPushConstants pushConstants{};
        pushConstants.model = model;
        pushConstants.baseColor = baseTint;
        if (isBlockedGhost) {
            pushConstants.baseColor = glm::vec4(1.0f, 0.2f, 0.2f, 1.0f);
        } else if (isGhost) {
            pushConstants.baseColor.a = 0.4f;
        }

        if (m_PipelineLayout != VK_NULL_HANDLE) {
            vkCmdPushConstants(commandBuffer, m_PipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &pushConstants);
        }
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout,
                                0, 1, &m_DefaultDescriptorSet, 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(meshComp.mesh->Indices.size()), 1, 0, 0, 0);
    } else {
        for (const auto &sub : meshComp.mesh->SubMeshes) {
            glm::mat4 model = transform.GetMatrix();

            MeshPushConstants pushConstants{};
            pushConstants.model = model;
            pushConstants.baseColor = baseTint;

            if (isBlockedGhost) {
                pushConstants.baseColor = glm::vec4(1.0f, 0.2f, 0.2f, 1.0f);
            } else if (registry.any_of<BeltComponent>(entity)) {
                pushConstants.baseColor = baseTint;
                if (isGhost) pushConstants.baseColor.a = 0.4f;
            } else {
                pushConstants.baseColor = (sub.materialIndex >= 0 && sub.materialIndex < (int)meshComp.mesh->Materials.size()) ? meshComp.mesh->Materials[sub.materialIndex].baseColor : glm::vec4(1.0f);
                if (isGhost) pushConstants.baseColor.a = 0.4f;
            }

            if (m_PipelineLayout != VK_NULL_HANDLE) {
                vkCmdPushConstants(commandBuffer, m_PipelineLayout,
                                   VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &pushConstants);
            }

            VkDescriptorSet setToBind = (sub.materialIndex >= 0 && sub.materialIndex < (int)meshComp.mesh->Materials.size() && meshComp.mesh->Materials[sub.materialIndex].descriptorSet != VK_NULL_HANDLE) ? meshComp.mesh->Materials[sub.materialIndex].descriptorSet : m_DefaultDescriptorSet;

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout,
                                    0, 1, &setToBind, 0, nullptr);

            vkCmdDrawIndexed(commandBuffer, sub.indexCount, 1, sub.indexOffset, 0, 0);
        }
    }
}


void MeshRenderer::CreateUniformBuffers() {


    VkDeviceSize lightingBufferSize = sizeof(LightingUBO);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_Engine->CreateBuffer(lightingBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_LightingUBOBuffers[i], m_LightingUBOMemory[i]);
        vkMapMemory(m_Engine->GetDevice(), m_LightingUBOMemory[i], 0, lightingBufferSize, 0, &m_LightingUBOMapped[i]);
    }

}


void MeshRenderer::CreatePipeline() {
    std::cout << "Creating Mesh Renderer pipeline..." << std::endl;
    // Push constant range for sprite transform and color
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(MeshPushConstants);


    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 0;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.descriptorCount = 1;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo textureLayoutInfo{};
    textureLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    textureLayoutInfo.bindingCount = 1;             // back to 1 — just the sampler, not merged with lighting
    textureLayoutInfo.pBindings = &samplerBinding;
    vkCreateDescriptorSetLayout(m_Engine->GetDevice(), &textureLayoutInfo, nullptr, &m_TextureDescriptorSetLayout);

    VkDescriptorPoolSize texturePoolSize{};
    texturePoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texturePoolSize.descriptorCount = 256;   // however many unique textured materials you expect

    VkDescriptorPoolCreateInfo texturePoolInfo{};
    texturePoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    texturePoolInfo.poolSizeCount = 1;
    texturePoolInfo.pPoolSizes = &texturePoolSize;
    texturePoolInfo.maxSets = 256;
    texturePoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    vkCreateDescriptorPool(m_Engine->GetDevice(), &texturePoolInfo, nullptr, &m_TextureDescriptorPool);

    // --- Set 1: lighting UBO + shadow map ---
    VkDescriptorSetLayoutBinding lightingBinding{};
    lightingBinding.binding = 0;
    lightingBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightingBinding.descriptorCount = 1;
    lightingBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;   // CHANGED — vertex now needs lightSpaceMatrix too

    VkDescriptorSetLayoutBinding shadowMapBinding{};                                            // NEW
    shadowMapBinding.binding = 1;
    shadowMapBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    shadowMapBinding.descriptorCount = 1;
    shadowMapBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding lightingBindings[] = { lightingBinding, shadowMapBinding };     // CHANGED — array of 2 now

    VkDescriptorSetLayoutCreateInfo lightingLayoutInfo{};
    lightingLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    lightingLayoutInfo.bindingCount = 2;                                                         // CHANGED — was 1
    lightingLayoutInfo.pBindings = lightingBindings;                                             // CHANGED — was &lightingBinding
    vkCreateDescriptorSetLayout(m_Engine->GetDevice(), &lightingLayoutInfo, nullptr, &m_LightingDescriptorSetLayout);

    VkDescriptorPoolSize lightingPoolSizes[2]{};                                                 // CHANGED — was single poolSize
    lightingPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightingPoolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;
    lightingPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    lightingPoolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo lightingPoolInfo{};
    lightingPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    lightingPoolInfo.poolSizeCount = 2;                                                          // CHANGED — was 1
    lightingPoolInfo.pPoolSizes = lightingPoolSizes;                                              // CHANGED
    lightingPoolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
    vkCreateDescriptorPool(m_Engine->GetDevice(), &lightingPoolInfo, nullptr, &m_LightingDescriptorPool);

    VkDescriptorSetLayout setLayouts[] = { m_TextureDescriptorSetLayout, m_LightingDescriptorSetLayout };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 2;
    pipelineLayoutInfo.pSetLayouts = setLayouts;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    vkCreatePipelineLayout(m_Engine->GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout);



    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 256; // max unique textured materials — raise if you have more

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 256;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    vkCreateDescriptorPool(m_Engine->GetDevice(), &poolInfo, nullptr, &m_TextureDescriptorPool);

    // Default 1x1 white texture/sampler for materials without a real texture — avoids needing two shader paths
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = samplerInfo.addressModeV = samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    vkCreateSampler(m_Engine->GetDevice(), &samplerInfo, nullptr, &m_DefaultSampler);




    if (vkCreatePipelineLayout(m_Engine->GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Mesh Renderer pipeline layout");
    }
    else {
        std::cout << "Mesh Renderer pipeline layout created successfully" << std::endl;
    }

    // Load shaders
    auto vertCode = m_Engine->ReadFile("Shaders/mesh_vert.spv");
    auto fragCode = m_Engine->ReadFile("Shaders/mesh_frag.spv");
    std::cout << "Loading Shader files..." << std::endl;
    VkShaderModule vertShaderModule = m_Engine->CreateShaderModule(vertCode);
    VkShaderModule fragShaderModule = m_Engine->CreateShaderModule(fragCode);
    std::cout << "Creating Shader modules..." << std::endl;
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";
    std::cout << "Vertex shader info loaded..." << std::endl;
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";
    std::cout << "Fragment shader info loaded..." << std::endl;

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
    std::cout << "Shader pipeline stages created..." << std::endl;
    // Vertex input
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkExtent2D extent = m_Engine->GetSwapChainExtent();

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    //VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;


    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Enable alpha blending for UI
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;



    m_SwapChainImageFormat = m_Engine->GetSwapChainFormat();


    VkPipelineRenderingCreateInfo pipelineRenderingInfo{};
    pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingInfo.colorAttachmentCount = 1;
    pipelineRenderingInfo.pColorAttachmentFormats = &m_SwapChainImageFormat;
    pipelineRenderingInfo.depthAttachmentFormat = m_Engine->GetDepthFormat();
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;


 

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &pipelineRenderingInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = m_PipelineLayout;
    pipelineInfo.renderPass = VK_NULL_HANDLE;   // dynamic rendering
    pipelineInfo.subpass = 0;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.pDepthStencilState = &depthStencil;

    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    if (vkCreateGraphicsPipelines(m_Engine->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Mesh Renderer graphics pipeline");
    }

    rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
    rasterizer.cullMode = VK_CULL_MODE_NONE;   // wireframe usually looks better without backface culling
    if (vkCreateGraphicsPipelines(m_Engine->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_WireframePipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Mesh Renderer wireframe pipeline");
    }

    vkDestroyShaderModule(m_Engine->GetDevice(), fragShaderModule, nullptr);
    vkDestroyShaderModule(m_Engine->GetDevice(), vertShaderModule, nullptr);

    std::cout << "Mesh Renderer pipelines created successfully" << std::endl;

}

void MeshRenderer::Shutdown() {
    VkDevice device = m_Engine->GetDevice();
    if (!m_initialized) {
        return;
    }
    vkDeviceWaitIdle(device);

    if (m_Pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_Pipeline, nullptr);
    }
    if (m_WireframePipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_WireframePipeline, nullptr);
    }
    if (m_PipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
    }
    // NEW — descriptor infrastructure
        if (m_TextureDescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, m_TextureDescriptorPool, nullptr);
        }
    if (m_TextureDescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_TextureDescriptorSetLayout, nullptr);
    }
    if (m_LightingDescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, m_LightingDescriptorPool, nullptr);
    }
    if (m_LightingDescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_LightingDescriptorSetLayout, nullptr);
    }

    // NEW — default texture + sampler
    if (m_DefaultTexture) {
        m_DefaultTexture->DestroyGPUResources(device);
    }
    if (m_DefaultSampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, m_DefaultSampler, nullptr);
    }

    // NEW — lighting UBOs, one per frame-in-flight
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (m_LightingUBOBuffers[i] != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, m_LightingUBOBuffers[i], nullptr);
            vkFreeMemory(device, m_LightingUBOMemory[i], nullptr);
        }
    }

    m_initialized = false;
    std::cout << "Mesh Renderer shut down" << std::endl;
}
VkDescriptorSet MeshRenderer::AllocateTextureDescriptorSet(VkImageView imageView, VkSampler sampler) {
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_TextureDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_TextureDescriptorSetLayout;

    VkDescriptorSet descriptorSet;
    if (vkAllocateDescriptorSets(m_Engine->GetDevice(), &allocInfo, &descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate texture descriptor set");
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptorSet;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_Engine->GetDevice(), 1, &write, 0, nullptr);
    return descriptorSet;
}

VkVertexInputBindingDescription MeshRenderer::GetInstanceBindingDescription() {
    VkVertexInputBindingDescription binding{};
    binding.binding = 1; // binding 1 — mesh vertices are binding 0
    binding.stride = sizeof(InstanceData);
    binding.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE; // KEY — advances per INSTANCE, not per vertex
    return binding;
}

std::array<VkVertexInputAttributeDescription, 5> MeshRenderer::GetInstanceAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 5> attrs{};

    // model matrix — 4 consecutive vec4 locations (continuing from wherever mesh vertex attributes left off)
    attrs[0] = { 6, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(InstanceData, model) + 0 * sizeof(glm::vec4) };
    attrs[1] = { 7, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(InstanceData, model) + 1 * sizeof(glm::vec4) };
    attrs[2] = { 8, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(InstanceData, model) + 2 * sizeof(glm::vec4) };
    attrs[3] = { 9, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(InstanceData, model) + 3 * sizeof(glm::vec4) };
    attrs[4] = { 10, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(InstanceData, tintColor) };

    return attrs;
}

void MeshRenderer::CreateInstancedPipeline() {
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    VkDescriptorSetLayout setLayouts[] = { m_TextureDescriptorSetLayout, m_LightingDescriptorSetLayout };
    pipelineLayoutInfo.setLayoutCount = 2;
    pipelineLayoutInfo.pSetLayouts = setLayouts;

    VkPushConstantRange pushConstantRange{}; // ADD
    pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // ADD — only fragment stage needs it
    pushConstantRange.offset = 0; // ADD
    pushConstantRange.size = sizeof(glm::vec4); // ADD — just baseColor

    pipelineLayoutInfo.pushConstantRangeCount = 1; // CHANGED — was 0
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange; // ADD

    vkCreatePipelineLayout(m_Engine->GetDevice(), &pipelineLayoutInfo, nullptr, &m_InstancedPipelineLayout);


    auto vertCode = m_Engine->ReadFile("Shaders/mesh_instanced_vert.spv");
    auto fragCode = m_Engine->ReadFile("Shaders/mesh_instanced_frag.spv");
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

    // Two bindings: mesh vertices (0) + instance data (1)
    auto meshBinding = Vertex::getBindingDescription();
    auto meshAttrs = Vertex::getAttributeDescriptions();
    auto instanceBinding = GetInstanceBindingDescription();
    auto instanceAttrs = GetInstanceAttributeDescriptions();

    VkVertexInputBindingDescription bindings[] = { meshBinding, instanceBinding };

    std::vector<VkVertexInputAttributeDescription> allAttrs;
    for (auto &a : meshAttrs)
        allAttrs.push_back(a);
    for (auto &a : instanceAttrs)
        allAttrs.push_back(a);

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
    pipelineInfo.layout = m_InstancedPipelineLayout;
    pipelineInfo.renderPass = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(m_Engine->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_InstancedPipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create instanced mesh pipeline");
    }

    vkDestroyShaderModule(m_Engine->GetDevice(), fragModule, nullptr);
    vkDestroyShaderModule(m_Engine->GetDevice(), vertModule, nullptr);
}
void MeshRenderer::EnsureInstanceBufferCapacity(size_t instanceCount) {
    if (instanceCount <= m_InstanceBufferCapacity) return;

    m_Engine->WaitIdle(); // ensure no in-flight command buffer is still using the old buffer before destroying it

    VkDevice device = m_Engine->GetDevice();
    if (m_InstanceBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_InstanceBuffer, nullptr);
        vkFreeMemory(device, m_InstanceBufferMemory, nullptr);
    }

    size_t newCapacity = std::max(instanceCount, m_InstanceBufferCapacity * 2);
    VkDeviceSize bufferSize = newCapacity * sizeof(InstanceData);

    m_Engine->CreateBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                           m_InstanceBuffer, m_InstanceBufferMemory);
    vkMapMemory(device, m_InstanceBufferMemory, 0, bufferSize, 0, &m_InstanceBufferMapped);

    m_InstanceBufferCapacity = newCapacity;
}

void MeshRenderer::DrawInstancedGroup(entt::registry &registry, Mesh *mesh, const std::vector<entt::entity> &entities,
                                      VkCommandBuffer commandBuffer) {
    std::vector<InstanceData> instances;
    instances.reserve(entities.size());

    for (auto entity : entities) {
        auto &transform = registry.get<TransformComponent>(entity);
        InstanceData data;
        data.model = transform.GetMatrix();
        data.tintColor = glm::vec4(1.0f);
        instances.push_back(data);
    }

    size_t writeOffsetElements = m_InstanceBufferWriteOffset;
    size_t writeOffsetBytes = writeOffsetElements * sizeof(InstanceData);
    memcpy(static_cast<char *>(m_InstanceBufferMapped) + writeOffsetBytes,
           instances.data(), instances.size() * sizeof(InstanceData));

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_InstancedPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_InstancedPipelineLayout,
                            1, 1, &m_LightingDescriptorSets[m_Engine->GetCurrentFrameIndex()], 0, nullptr);

    VkDeviceSize instanceBufferOffset = writeOffsetBytes;
    VkBuffer vertexBuffers[] = { mesh->vertexBuffer, m_InstanceBuffer };
    VkDeviceSize offsets[] = { 0, instanceBufferOffset };
    vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    if (mesh->SubMeshes.empty()) {
        glm::vec4 baseColor = glm::vec4(1.0f);
        vkCmdPushConstants(commandBuffer, m_InstancedPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(glm::vec4), &baseColor);

        VkDescriptorSet setToBind = m_DefaultDescriptorSet;
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_InstancedPipelineLayout,
                                0, 1, &setToBind, 0, nullptr);
       // std::cout << "About to draw instanced, pipeline=" << m_InstancedPipeline << " layout=" << m_InstancedPipelineLayout << std::endl;
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh->Indices.size()), static_cast<uint32_t>(instances.size()), 0, 0, 0);
    } else {
        for (const auto &sub : mesh->SubMeshes) {
            glm::vec4 subColor = (sub.materialIndex >= 0 && sub.materialIndex < (int)mesh->Materials.size()) ? mesh->Materials[sub.materialIndex].baseColor : glm::vec4(1.0f);
            vkCmdPushConstants(commandBuffer, m_InstancedPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
                               0, sizeof(glm::vec4), &subColor);

            VkDescriptorSet setToBind = (sub.materialIndex >= 0 && sub.materialIndex < (int)mesh->Materials.size() && mesh->Materials[sub.materialIndex].descriptorSet != VK_NULL_HANDLE) ? mesh->Materials[sub.materialIndex].descriptorSet : m_DefaultDescriptorSet;

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_InstancedPipelineLayout,
                                    0, 1, &setToBind, 0, nullptr);
            //std::cout << "About to draw instanced, pipeline=" << m_InstancedPipeline << " layout=" << m_InstancedPipelineLayout << std::endl;
            vkCmdDrawIndexed(commandBuffer, sub.indexCount, static_cast<uint32_t>(instances.size()), sub.indexOffset, 0, 0);
        }
    }

    m_InstanceBufferWriteOffset += instances.size(); // advance past this group's region for the next group
}