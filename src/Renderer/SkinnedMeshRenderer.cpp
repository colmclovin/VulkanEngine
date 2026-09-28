// SkinnedMeshRenderer.cpp — key pieces
#include "SkinnedMeshRenderer.h"
#include "../Rendering/Texture.h"
#include "../Utils/TextureLoader.h"
#include "../Engine/VulkanEngine.h"
#include "../Game/Camera3D.h"
#include "../Components/Components.h"
#include "../Components/SkinnedMeshComponent.h"
#include "../Rendering/SkinnedVertex.h"
#include "../Rendering/SkinnedMesh.h"
#include <cstring>
#include <stdexcept>
#include "../Rendering/BoneMatrixUBO.h"
#include "../Rendering/LightingUBO.h"
#include "../Rendering/Animator.h"
#include "../Rendering/Frustum.h"
#include <iostream>
#include "ShadowMap.h"
SkinnedMeshRenderer::SkinnedMeshRenderer(VulkanEngine *engine) : m_Engine(engine) {}

void SkinnedMeshRenderer::Init(ShadowMap* shadowMap) {
    m_ShadowMap = shadowMap;
    CreatePipeline();
    CreateUniformBuffers();

    // Default white texture (binding 1), same pattern as MeshRenderer
    TextureData whitePixel;
    whitePixel.width = 1;
    whitePixel.height = 1;
    whitePixel.pixels = { 255, 255, 255, 255 };
    m_DefaultTexture = std::make_shared<Texture>();
    m_DefaultTexture->UploadToGPU(m_Engine, whitePixel);

    // Allocate + write descriptor sets (UBO + texture) per frame-in-flight
    VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT] = { m_DescriptorSetLayout, m_DescriptorSetLayout };
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts;
    vkAllocateDescriptorSets(m_Engine->GetDevice(), &allocInfo, m_DescriptorSets);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo boneBufferInfo{};
        boneBufferInfo.buffer = m_BoneUBOBuffers[i];
        boneBufferInfo.offset = 0;
        boneBufferInfo.range = sizeof(BoneMatrixUBO);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_DefaultTexture->imageView;
        imageInfo.sampler = m_DefaultTexture->sampler;

        VkDescriptorBufferInfo lightingBufferInfo{};
        lightingBufferInfo.buffer = m_LightingUBOBuffers[i];
        lightingBufferInfo.offset = 0;
        lightingBufferInfo.range = sizeof(LightingUBO);

        VkDescriptorImageInfo shadowMapInfo{};   // ADD THIS — was missing
        shadowMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        shadowMapInfo.imageView = m_ShadowMap->GetImageView();
        shadowMapInfo.sampler = m_ShadowMap->GetSampler();


        VkWriteDescriptorSet writes[4]{};


        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = m_DescriptorSets[i];
        writes[0].dstBinding = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].descriptorCount = 1;
        writes[0].pBufferInfo = &boneBufferInfo;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = m_DescriptorSets[i];
        writes[1].dstBinding = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].descriptorCount = 1;
        writes[1].pImageInfo = &imageInfo;

        writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[2].dstSet = m_DescriptorSets[i];
        writes[2].dstBinding = 2;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[2].descriptorCount = 1;
        writes[2].pBufferInfo = &lightingBufferInfo;

        writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;   // NEW
        writes[3].dstSet = m_DescriptorSets[i];
        writes[3].dstBinding = 3;
        writes[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[3].descriptorCount = 1;
        writes[3].pImageInfo = &shadowMapInfo;

        vkUpdateDescriptorSets(m_Engine->GetDevice(), 4, writes, 0, nullptr);
    }
    VkDescriptorSetLayout shadowLayouts[MAX_FRAMES_IN_FLIGHT] = { m_ShadowDescriptorSetLayout, m_ShadowDescriptorSetLayout };
    VkDescriptorSetAllocateInfo shadowAllocInfo{};
    shadowAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    shadowAllocInfo.descriptorPool = m_ShadowDescriptorPool;
    shadowAllocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    shadowAllocInfo.pSetLayouts = shadowLayouts;
    vkAllocateDescriptorSets(m_Engine->GetDevice(), &shadowAllocInfo, m_ShadowDescriptorSets);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo shadowBoneBufferInfo{};
        shadowBoneBufferInfo.buffer = m_BoneUBOBuffers[i]; // SAME buffer the main pipeline already writes each frame
        shadowBoneBufferInfo.offset = 0;
        shadowBoneBufferInfo.range = sizeof(BoneMatrixUBO);

        VkWriteDescriptorSet shadowWrite{};
        shadowWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        shadowWrite.dstSet = m_ShadowDescriptorSets[i];
        shadowWrite.dstBinding = 0;
        shadowWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        shadowWrite.descriptorCount = 1;
        shadowWrite.pBufferInfo = &shadowBoneBufferInfo;

        vkUpdateDescriptorSets(m_Engine->GetDevice(), 1, &shadowWrite, 0, nullptr);
    }

    m_initialized = true;
    std::cout << "Skinned Mesh Renderer initialized" << std::endl;
}

void SkinnedMeshRenderer::CreateUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(BoneMatrixUBO);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_Engine->CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                               m_BoneUBOBuffers[i], m_BoneUBOMemory[i]);
        vkMapMemory(m_Engine->GetDevice(), m_BoneUBOMemory[i], 0, bufferSize, 0, &m_BoneUBOMapped[i]);
        // Left mapped persistently — standard pattern for frequently-updated UBOs, avoids map/unmap every frame
    }

    VkDeviceSize lightingBufferSize = sizeof(LightingUBO);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_Engine->CreateBuffer(lightingBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_LightingUBOBuffers[i], m_LightingUBOMemory[i]);
        vkMapMemory(m_Engine->GetDevice(), m_LightingUBOMemory[i], 0, lightingBufferSize, 0, &m_LightingUBOMapped[i]);
    }

}
// SkinnedMeshRenderer.cpp
void SkinnedMeshRenderer::CreatePipeline() {
    // --- Descriptor set layout: binding 0 = bone UBO, binding 1 = texture sampler ---
    VkDescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 1;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.descriptorCount = 1;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


    VkDescriptorSetLayoutBinding lightingBinding{};
    lightingBinding.binding = 2;   // check this doesn't collide with your existing sampler binding
    lightingBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightingBinding.descriptorCount = 1;
    lightingBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;   // must include BOTH

    VkDescriptorSetLayoutBinding shadowMapBinding{};   // NEW
    shadowMapBinding.binding = 3;
    shadowMapBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    shadowMapBinding.descriptorCount = 1;
    shadowMapBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


    VkDescriptorSetLayoutBinding bindings[] = { uboBinding, samplerBinding, lightingBinding, shadowMapBinding };

    VkDescriptorSetLayoutCreateInfo setLayoutInfo{};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.bindingCount = 4;
    setLayoutInfo.pBindings = bindings;
    vkCreateDescriptorSetLayout(m_Engine->GetDevice(), &setLayoutInfo, nullptr, &m_DescriptorSetLayout);

    // --- Descriptor pool ---
    VkDescriptorPoolSize poolSizes[2]{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT * 2;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT * 2;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
    vkCreateDescriptorPool(m_Engine->GetDevice(), &poolInfo, nullptr, &m_DescriptorPool);

    // --- Push constants: same MVP + baseColor pattern as MeshRenderer ---
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4) + sizeof(glm::vec4); // mvp + baseColor, matching MeshPushConstants shape

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    vkCreatePipelineLayout(m_Engine->GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout);

    // --- Shaders ---
    auto vertCode = m_Engine->ReadFile("Shaders/skinned_vert.spv");
    auto fragCode = m_Engine->ReadFile("Shaders/skinned_frag.spv");
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

    // --- Vertex input: SkinnedVertex's own binding/attributes ---
    auto bindingDescription = SkinnedVertex::getBindingDescription();
    auto attributeDescriptions = SkinnedVertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

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
        throw std::runtime_error("Failed to create Skinned Mesh Renderer graphics pipeline");
    }

    rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
    rasterizer.cullMode = VK_CULL_MODE_NONE;   // wireframe usually looks better without backface culling
    if (vkCreateGraphicsPipelines(m_Engine->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_WireframePipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create skinned mesh wireframe pipeline");
    }

    vkDestroyShaderModule(m_Engine->GetDevice(), fragModule, nullptr);
    vkDestroyShaderModule(m_Engine->GetDevice(), vertModule, nullptr);




        VkDescriptorSetLayoutBinding shadowUboBinding{};
    shadowUboBinding.binding = 0;
    shadowUboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    shadowUboBinding.descriptorCount = 1;
    shadowUboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo shadowSetLayoutInfo{};
    shadowSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    shadowSetLayoutInfo.bindingCount = 1;
    shadowSetLayoutInfo.pBindings = &shadowUboBinding;
    vkCreateDescriptorSetLayout(m_Engine->GetDevice(), &shadowSetLayoutInfo, nullptr, &m_ShadowDescriptorSetLayout);

    VkDescriptorPoolSize shadowPoolSize{};
    shadowPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    shadowPoolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo shadowPoolInfo{};
    shadowPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    shadowPoolInfo.poolSizeCount = 1;
    shadowPoolInfo.pPoolSizes = &shadowPoolSize;
    shadowPoolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
    vkCreateDescriptorPool(m_Engine->GetDevice(), &shadowPoolInfo, nullptr, &m_ShadowDescriptorPool);

    VkPushConstantRange shadowPushRange{};
    shadowPushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    shadowPushRange.offset = 0;
    shadowPushRange.size = sizeof(glm::mat4) * 2; // model + lightSpaceMatrix

    VkPipelineLayoutCreateInfo shadowPipelineLayoutInfo{};
    shadowPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    shadowPipelineLayoutInfo.setLayoutCount = 1;
    shadowPipelineLayoutInfo.pSetLayouts = &m_ShadowDescriptorSetLayout;
    shadowPipelineLayoutInfo.pushConstantRangeCount = 1;
    shadowPipelineLayoutInfo.pPushConstantRanges = &shadowPushRange;
    vkCreatePipelineLayout(m_Engine->GetDevice(), &shadowPipelineLayoutInfo, nullptr, &m_ShadowPipelineLayout);

    // Load the new shadow shaders
    auto shadowVertCode = m_Engine->ReadFile("Shaders/shadow_skinned_vert.spv");
    auto shadowFragCode = m_Engine->ReadFile("Shaders/shadow_skinned_frag.spv");
    VkShaderModule shadowVertModule = m_Engine->CreateShaderModule(shadowVertCode);
    VkShaderModule shadowFragModule = m_Engine->CreateShaderModule(shadowFragCode);

    VkPipelineShaderStageCreateInfo shadowVertStage{};
    shadowVertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shadowVertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    shadowVertStage.module = shadowVertModule;
    shadowVertStage.pName = "main";

    VkPipelineShaderStageCreateInfo shadowFragStage{};
    shadowFragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shadowFragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shadowFragStage.module = shadowFragModule;
    shadowFragStage.pName = "main";

    VkPipelineShaderStageCreateInfo shadowStages[] = { shadowVertStage, shadowFragStage };

    // Reuse SkinnedVertex's real binding/attribute layout — same vertex buffer as the main pipeline uses
    auto shadowBindingDesc = SkinnedVertex::getBindingDescription();
    auto shadowAttrDescs = SkinnedVertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo shadowVertexInputInfo{};
    shadowVertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    shadowVertexInputInfo.vertexBindingDescriptionCount = 1;
    shadowVertexInputInfo.pVertexBindingDescriptions = &shadowBindingDesc;
    shadowVertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(shadowAttrDescs.size());
    shadowVertexInputInfo.pVertexAttributeDescriptions = shadowAttrDescs.data();

    VkPipelineInputAssemblyStateCreateInfo shadowInputAssembly{};
    shadowInputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    shadowInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo shadowViewportState{};
    shadowViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    shadowViewportState.viewportCount = 1;
    shadowViewportState.scissorCount = 1;

    VkDynamicState shadowDynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo shadowDynamicState{};
    shadowDynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    shadowDynamicState.dynamicStateCount = 2;
    shadowDynamicState.pDynamicStates = shadowDynamicStates;

    VkPipelineRasterizationStateCreateInfo shadowRasterizer{};
    shadowRasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    shadowRasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    shadowRasterizer.lineWidth = 1.0f;
    shadowRasterizer.cullMode = VK_CULL_MODE_FRONT_BIT; // same peter-panning/acne trick as the static shadow pipeline
    shadowRasterizer.depthBiasEnable = VK_TRUE;

    VkPipelineMultisampleStateCreateInfo shadowMultisampling{};
    shadowMultisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    shadowMultisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo shadowDepthStencil{};
    shadowDepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    shadowDepthStencil.depthTestEnable = VK_TRUE;
    shadowDepthStencil.depthWriteEnable = VK_TRUE;
    shadowDepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendStateCreateInfo shadowColorBlending{};
    shadowColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    shadowColorBlending.attachmentCount = 0; // depth-only, no color output

    VkPipelineRenderingCreateInfo shadowRenderingInfo{};
    shadowRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    shadowRenderingInfo.colorAttachmentCount = 0;
    shadowRenderingInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT; // must match ShadowMap's format

    VkGraphicsPipelineCreateInfo shadowPipelineInfo{};
    shadowPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    shadowPipelineInfo.pNext = &shadowRenderingInfo;
    shadowPipelineInfo.stageCount = 2;
    shadowPipelineInfo.pStages = shadowStages;
    shadowPipelineInfo.pVertexInputState = &shadowVertexInputInfo;
    shadowPipelineInfo.pInputAssemblyState = &shadowInputAssembly;
    shadowPipelineInfo.pViewportState = &shadowViewportState;
    shadowPipelineInfo.pRasterizationState = &shadowRasterizer;
    shadowPipelineInfo.pMultisampleState = &shadowMultisampling;
    shadowPipelineInfo.pDepthStencilState = &shadowDepthStencil;
    shadowPipelineInfo.pColorBlendState = &shadowColorBlending;
    shadowPipelineInfo.pDynamicState = &shadowDynamicState;
    shadowPipelineInfo.layout = m_ShadowPipelineLayout;
    shadowPipelineInfo.renderPass = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(m_Engine->GetDevice(), VK_NULL_HANDLE, 1, &shadowPipelineInfo, nullptr, &m_ShadowPipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create skinned shadow pipeline");
    }

    vkDestroyShaderModule(m_Engine->GetDevice(), shadowFragModule, nullptr);
    vkDestroyShaderModule(m_Engine->GetDevice(), shadowVertModule, nullptr);


    std::cout << "Skinned Mesh Renderer pipelines created successfully" << std::endl;
}

void SkinnedMeshRenderer::Render(entt::registry &registry, const Camera3D &camera, bool wireframe, DayNightCycle &dayNightCycle, const glm::mat4 &lightSpaceMatrix, const std::vector<PointLight> &activeLights) {
    VkCommandBuffer commandBuffer = m_Engine->GetCurrentCommandBuffer();
    uint32_t frameIndex = m_Engine->GetCurrentFrameIndex();

    VkExtent2D extent = m_Engine->GetSwapChainExtent();
    float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);

    VkViewport viewport{ 0.0f, 0.0f, (float)extent.width, (float)extent.height, 0.0f, 1.0f };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    VkRect2D scissor{ { 0, 0 }, extent };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    glm::mat4 view = camera.GetActiveViewMatrix();
    glm::mat4 proj = camera.GetProjectionMatrix(aspect);

    Frustum frustum = Frustum::FromViewProj(proj * view);
    LightingUBO lighting{};
    lighting.viewProj = proj * view;   // is this line actually present?
    lighting.sunDirection = glm::vec4(dayNightCycle.GetSunDirection(), 0.0f);
    lighting.sunColor = glm::vec4(dayNightCycle.GetSunColor(), dayNightCycle.GetAmbientIntensity());
    lighting.lightSpaceMatrix = lightSpaceMatrix;
    lighting.numPointLights = static_cast<int>(activeLights.size());
    for (size_t i = 0; i < activeLights.size(); i++) {
        lighting.pointLights[i] = activeLights[i];
    }
    memcpy(m_LightingUBOMapped[frameIndex], &lighting, sizeof(LightingUBO));

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout,
        0, 1, &m_DescriptorSets[frameIndex], 0, nullptr);

    VkPipeline pipelineToUse = wireframe ? m_WireframePipeline : m_Pipeline;
    if (pipelineToUse != VK_NULL_HANDLE) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineToUse);
    }
    

    auto view3D = registry.view<TransformComponent, SkinnedMeshComponent>();
    for (auto entity : view3D) {
        auto &transform = view3D.get<TransformComponent>(entity);
        auto &meshComp = view3D.get<SkinnedMeshComponent>(entity);

        if (!meshComp.mesh) continue;
        meshComp.mesh->UploadToGPU(m_Engine);

        float radius = 5.0f;   // ADD — rough bounding radius; use BoundsComponent if present for accuracy
        if (registry.any_of<BoundsComponent>(entity)) {
            auto& bounds = registry.get<BoundsComponent>(entity);
            radius = glm::length(bounds.halfExtents);
        }
        if (!frustum.ContainsSphere(transform.Position, radius)) continue;   // ADD — skip if off-screen

        meshComp.mesh->UploadToGPU(m_Engine);
        // --- Stage C: compute this entity's bone matrices for the current test pose ---
        BoneMatrixUBO uboData{};
        for (auto &m : uboData.boneMatrices)
            m = glm::mat4(1.0f);
        
        if (!meshComp.mesh->animations.empty() && registry.any_of<AnimationComponent>(entity)) {
            auto &anim = registry.get<AnimationComponent>(entity);
            if (anim.currentClipIndex >= 0 && anim.currentClipIndex < (int)meshComp.mesh->animations.size()) {
                const AnimationClip &clip = meshComp.mesh->animations[anim.currentClipIndex];
                auto pose = Animator::ComputePose(meshComp.mesh->skeleton, clip, anim.playbackTime);

                for (size_t i = 0; i < pose.size() && i < MAX_BONES; i++) {
                    uboData.boneMatrices[i] = pose[i];
                }
            }

        }
        

        memcpy(m_BoneUBOMapped[frameIndex], &uboData, sizeof(BoneMatrixUBO));

        // Descriptor set bind moved here too, since it must be re-bound after each UBO update
        // (technically the UBO is host-coherent so the GPU sees updates without a fresh bind,
        // but binding here keeps the code straightforward and correct per-entity)
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout,
                                0, 1, &m_DescriptorSets[frameIndex], 0, nullptr);

        VkBuffer vertexBuffers[] = { meshComp.mesh->vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, meshComp.mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

      

        struct {
            glm::mat4 model;
            glm::vec4 baseColor;
        } pushConstants;
        pushConstants.model = transform.GetMatrix();   // CHANGED — just the model matrix, no view/proj baked in
        pushConstants.baseColor = glm::vec4(1.0f);

        vkCmdPushConstants(commandBuffer, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(meshComp.mesh->Indices.size()), 1, 0, 0, 0);
    }
}

void SkinnedMeshRenderer::RenderShadowPass(entt::registry &registry, const glm::mat4 &lightSpaceMatrix) {
    VkCommandBuffer commandBuffer = m_Engine->GetCurrentCommandBuffer();
    uint32_t frameIndex = m_Engine->GetCurrentFrameIndex();

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ShadowPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ShadowPipelineLayout,
                            0, 1, &m_ShadowDescriptorSets[frameIndex], 0, nullptr);

    auto view = registry.view<TransformComponent, SkinnedMeshComponent>();
    for (auto entity : view) {
        auto &transform = view.get<TransformComponent>(entity);
        auto &meshComp = view.get<SkinnedMeshComponent>(entity);
        if (!meshComp.mesh || !meshComp.mesh->IsUploaded()) continue;

        struct {
            glm::mat4 model;
            glm::mat4 lightSpaceMatrix;
        } pushConstants;
        pushConstants.model = transform.GetMatrix();
        pushConstants.lightSpaceMatrix = lightSpaceMatrix;

        vkCmdPushConstants(commandBuffer, m_ShadowPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);

        VkBuffer vertexBuffers[] = { meshComp.mesh->vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, meshComp.mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(meshComp.mesh->Indices.size()), 1, 0, 0, 0);
    }
}


void SkinnedMeshRenderer::Shutdown() {
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
    if (m_DescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
    }
    if (m_DescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
    }
    if (m_ShadowPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_ShadowPipeline, nullptr);
    }
    if (m_ShadowPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_ShadowPipelineLayout, nullptr);
    }
    if (m_ShadowDescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, m_ShadowDescriptorPool, nullptr);
    }
    if (m_ShadowDescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_ShadowDescriptorSetLayout, nullptr);
    }

    if (m_DefaultTexture) {
        m_DefaultTexture->DestroyGPUResources(device);
    }

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (m_BoneUBOBuffers[i] != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, m_BoneUBOBuffers[i], nullptr);
            vkFreeMemory(device, m_BoneUBOMemory[i], nullptr);
        }
        if (m_LightingUBOBuffers[i] != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, m_LightingUBOBuffers[i], nullptr);
            vkFreeMemory(device, m_LightingUBOMemory[i], nullptr);
        }
    }

    std::cout << "Skinned Mesh Renderer shut down" << std::endl;
}