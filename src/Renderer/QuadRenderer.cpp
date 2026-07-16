#include "QuadRenderer.h"
#include <iostream>
#include "../Components/Mesh.h"
#include "../Components/Vertex.h"
#include "../Engine/VulkanEngine.h"
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>
#include <glm/glm.hpp>
#include "../Components/Components.h"

QuadRenderer::QuadRenderer(VulkanEngine* engine) : m_Engine(engine) {
}

QuadRenderer::~QuadRenderer() {
    Shutdown();
}

void QuadRenderer::Init() {
    std::cout << "Initializing Quad Renderer..." << std::endl;
    CreateQuad();
    CreatePipeline();
    m_initialized = true;
    std::cout << "Quad Renderer initialized" << std::endl;
}

void QuadRenderer::Render(entt::registry &registry) {
    if (!m_QuadMesh) {
        return;
    }

    // Upload quad mesh to GPU if not already done
    if (!m_QuadUploaded && m_QuadMesh->vertexBuffer == VK_NULL_HANDLE) {
        VkDeviceSize vertexBufferSize = sizeof(m_QuadMesh->Vertices[0]) * m_QuadMesh->Vertices.size();
        VkDeviceSize indexBufferSize = sizeof(m_QuadMesh->Indices[0]) * m_QuadMesh->Indices.size();

        // Create and upload vertex buffer
        VkBuffer vertexStagingBuffer;
        VkDeviceMemory vertexStagingBufferMemory;
        m_Engine->CreateBuffer(
                vertexBufferSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                vertexStagingBuffer,
                vertexStagingBufferMemory);

        void *data;
        vkMapMemory(m_Engine->GetDevice(), vertexStagingBufferMemory, 0, vertexBufferSize, 0, &data);
        memcpy(data, m_QuadMesh->Vertices.data(), vertexBufferSize);
        vkUnmapMemory(m_Engine->GetDevice(), vertexStagingBufferMemory);

        m_Engine->CreateBuffer(
                vertexBufferSize,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                m_QuadMesh->vertexBuffer,
                m_QuadMesh->vertexBufferMemory);

        m_Engine->CopyBuffer(vertexStagingBuffer, m_QuadMesh->vertexBuffer, vertexBufferSize);

        vkDestroyBuffer(m_Engine->GetDevice(), vertexStagingBuffer, nullptr);
        vkFreeMemory(m_Engine->GetDevice(), vertexStagingBufferMemory, nullptr);

        // Create and upload index buffer
        VkBuffer indexStagingBuffer;
        VkDeviceMemory indexStagingBufferMemory;
        m_Engine->CreateBuffer(
                indexBufferSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                indexStagingBuffer,
                indexStagingBufferMemory);

        vkMapMemory(m_Engine->GetDevice(), indexStagingBufferMemory, 0, indexBufferSize, 0, &data);
        memcpy(data, m_QuadMesh->Indices.data(), indexBufferSize);
        vkUnmapMemory(m_Engine->GetDevice(), indexStagingBufferMemory);

        m_Engine->CreateBuffer(
                indexBufferSize,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                m_QuadMesh->indexBuffer,
                m_QuadMesh->indexBufferMemory);

        m_Engine->CopyBuffer(indexStagingBuffer, m_QuadMesh->indexBuffer, indexBufferSize);

        vkDestroyBuffer(m_Engine->GetDevice(), indexStagingBuffer, nullptr);
        vkFreeMemory(m_Engine->GetDevice(), indexStagingBufferMemory, nullptr);

        m_QuadUploaded = true;
        std::cout << "UI quad mesh uploaded to GPU" << std::endl;
    }

    VkCommandBuffer commandBuffer = m_Engine->GetCurrentCommandBuffer();

    // Bind pipeline
    if (m_Pipeline != VK_NULL_HANDLE) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
    }

    // Bind quad mesh
    if (m_QuadMesh->vertexBuffer != VK_NULL_HANDLE) {
        VkBuffer vertexBuffers[] = { m_QuadMesh->vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, m_QuadMesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    }

    // Get viewport dimensions for screen-space conversion
    VkExtent2D extent = m_Engine->GetSwapChainExtent();
    float screenWidth = static_cast<float>(extent.width);
    float screenHeight = static_cast<float>(extent.height);

    // Render all sprite components, sorted by layer
    auto view = registry.view<SpriteComponent>();

    // TODO: Sort by layer for proper rendering order
    for (auto entity : view) {
        auto &sprite = view.get<SpriteComponent>(entity);

        // Build transform matrix for screen-space rendering
        glm::mat4 transform = glm::mat4(1.0f);

        // Convert screen position to normalized device coordinates
        float ndcX = (sprite.position.x / screenWidth) * 2.0f - 1.0f;
        float ndcY = (sprite.position.y / screenHeight) * 2.0f - 1.0f;

        transform = glm::translate(transform, glm::vec3(ndcX, ndcY, 0.0f));
        transform = glm::rotate(transform, sprite.rotation, glm::vec3(0.0f, 0.0f, 1.0f));

        // Scale to screen size
        float scaleX = sprite.size.x / screenWidth * 2.0f;
        float scaleY = sprite.size.y / screenHeight * 2.0f;
        transform = glm::scale(transform, glm::vec3(scaleX, scaleY, 1.0f));

        SpritePushConstants pushConstants{};
        pushConstants.transform = transform;
        pushConstants.color = sprite.color;

        if (m_PipelineLayout != VK_NULL_HANDLE) {
            vkCmdPushConstants(
                    commandBuffer,
                    m_PipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    sizeof(SpritePushConstants),
                    &pushConstants);
        }

        // Draw the quad
        if (m_QuadMesh->indexBuffer != VK_NULL_HANDLE) {
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_QuadMesh->Indices.size()), 1, 0, 0, 0);
        }
    }
}

void QuadRenderer::CreateQuad() {

    m_QuadMesh = new Mesh();

    m_QuadMesh->Vertices = {
        { { -0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
        { { 0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
        { { 0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
        { { -0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } }
    };

    m_QuadMesh->Indices = { 0, 1, 2, 2, 3, 0 };

    std::cout << "Created quad mesh" << std::endl;
}

void QuadRenderer::CreatePipeline() {
    std::cout << "Creating Quad Renderer pipeline..." << std::endl;
    // Push constant range for sprite transform and color
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(SpritePushConstants);

    // Pipeline layout (no descriptor sets, just push constants)
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(m_Engine->GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Quad Renderer pipeline layout");
    } else {
        std::cout << "Quad Renderer pipeline layout created successfully" << std::endl;
    }

    // Load shaders
    auto vertCode = m_Engine->ReadFile("Shaders/ui_sprite_vert.spv");
    auto fragCode = m_Engine->ReadFile("Shaders/ui_sprite_frag.spv");
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
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = extent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE; // No culling for UI
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

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = m_PipelineLayout;
    pipelineInfo.renderPass = m_Engine->GetRenderPass();
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(m_Engine->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Quad Renderer graphics pipeline");
    }

    vkDestroyShaderModule(m_Engine->GetDevice(), fragShaderModule, nullptr);
    vkDestroyShaderModule(m_Engine->GetDevice(), vertShaderModule, nullptr);

    std::cout << "Quad Renderer pipeline created successfully" << std::endl;

}

void QuadRenderer::Shutdown() {
    VkDevice device = m_Engine->GetDevice();
	if (!m_initialized) {
		return;
	}
    vkDeviceWaitIdle(device);
    // Cleanup quad mesh
    if (m_QuadMesh) {
        if (m_QuadMesh->vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, m_QuadMesh->vertexBuffer, nullptr);
            vkFreeMemory(device, m_QuadMesh->vertexBufferMemory, nullptr);
        }
        if (m_QuadMesh->indexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, m_QuadMesh->indexBuffer, nullptr);
            vkFreeMemory(device, m_QuadMesh->indexBufferMemory, nullptr);
        }
        delete m_QuadMesh;
        m_QuadMesh = nullptr;
    }

    // Cleanup pipeline
    if (m_Pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_Pipeline, nullptr);
    }
    if (m_PipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
    }
    m_initialized = false;
    std::cout << "Quad Renderer shut down" << std::endl;
}
