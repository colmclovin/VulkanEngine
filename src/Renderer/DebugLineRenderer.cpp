// DebugLineRenderer.cpp
#include "DebugLineRenderer.h"
#include "../Engine/VulkanEngine.h"
#include "../Game/Camera3D.h"
#include <stdexcept>

DebugLineRenderer::DebugLineRenderer(VulkanEngine *engine) : m_Engine(engine) {}

void DebugLineRenderer::Init() {
    CreatePipeline();
}

void DebugLineRenderer::CreatePipeline() {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;
    vkCreatePipelineLayout(m_Engine->GetDevice(), &layoutInfo, nullptr, &m_PipelineLayout);

    auto vertCode = m_Engine->ReadFile("Shaders/debugline_vert.spv");
    auto fragCode = m_Engine->ReadFile("Shaders/debugline_frag.spv");
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

    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(DebugLineVertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrs[2]{};
    attrs[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(DebugLineVertex, position) };
    attrs[1] = { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(DebugLineVertex, color) };

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = 2;
    vertexInput.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; // KEY DIFFERENCE from your triangle pipelines

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
    rasterizer.cullMode = VK_CULL_MODE_NONE;

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
    depthStencil.depthWriteEnable = VK_FALSE; // lines shouldn't occlude solid geometry oddly
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
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_PipelineLayout;
    pipelineInfo.renderPass = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(m_Engine->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create debug line pipeline");
    }

    vkDestroyShaderModule(m_Engine->GetDevice(), fragModule, nullptr);
    vkDestroyShaderModule(m_Engine->GetDevice(), vertModule, nullptr);
}

void DebugLineRenderer::BeginFrame() {
    m_Lines.clear();
}

void DebugLineRenderer::AddLine(glm::vec3 a, glm::vec3 b, glm::vec3 color) {
    m_Lines.push_back({ a, color });
    m_Lines.push_back({ b, color });
}

void DebugLineRenderer::AddBox(glm::vec3 min, glm::vec3 max, glm::vec3 color) {
    glm::vec3 c[8] = {
        { min.x, min.y, min.z }, { max.x, min.y, min.z }, { max.x, min.y, max.z }, { min.x, min.y, max.z }, { min.x, max.y, min.z }, { max.x, max.y, min.z }, { max.x, max.y, max.z }, { min.x, max.y, max.z }
    };
    int edges[12][2] = { { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 }, { 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 }, { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } };
    for (auto &e : edges)
        AddLine(c[e[0]], c[e[1]], color);
}

void DebugLineRenderer::Render(const Camera3D &camera, float aspect) {
    if (m_Lines.empty()) return;

    VkDeviceSize requiredSize = m_Lines.size() * sizeof(DebugLineVertex);
    if (requiredSize > m_BufferCapacity) {
        // (Re)allocate a host-visible buffer sized to fit — simplest approach for a debug tool, not perf-critical.
        if (m_VertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_Engine->GetDevice(), m_VertexBuffer, nullptr);
            vkFreeMemory(m_Engine->GetDevice(), m_VertexBufferMemory, nullptr);
        }
        m_Engine->CreateBuffer(requiredSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                               m_VertexBuffer, m_VertexBufferMemory);
        m_BufferCapacity = requiredSize;
    }

    void *data;
    vkMapMemory(m_Engine->GetDevice(), m_VertexBufferMemory, 0, requiredSize, 0, &data);
    memcpy(data, m_Lines.data(), requiredSize);
    vkUnmapMemory(m_Engine->GetDevice(), m_VertexBufferMemory);

    VkCommandBuffer cmd = m_Engine->GetCurrentCommandBuffer();
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

    VkExtent2D extent = m_Engine->GetSwapChainExtent();
    VkViewport viewport{ 0, 0, (float)extent.width, (float)extent.height, 0, 1 };
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    VkRect2D scissor{ { 0, 0 }, extent };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    glm::mat4 viewProj = camera.GetProjectionMatrix(aspect) * camera.GetActiveViewMatrix();
    vkCmdPushConstants(cmd, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &viewProj);

    VkBuffer buffers[] = { m_VertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);
    vkCmdDraw(cmd, static_cast<uint32_t>(m_Lines.size()), 1, 0, 0);
}

void DebugLineRenderer::Shutdown() {
    if (m_VertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_Engine->GetDevice(), m_VertexBuffer, nullptr);
        vkFreeMemory(m_Engine->GetDevice(), m_VertexBufferMemory, nullptr);
    }
    vkDestroyPipeline(m_Engine->GetDevice(), m_Pipeline, nullptr);
    vkDestroyPipelineLayout(m_Engine->GetDevice(), m_PipelineLayout, nullptr);
}