#include "FogLayerRenderer.h"
#include "../Engine/VulkanEngine.h" // adjust path
#include "../Rendering/FogTexture.h"
#include <stdexcept>

void FogLayerRenderer::Init(FogTexture *fogTex) {
    VkDevice device = m_Engine->GetDevice();

    // Set layout: binding 0 = fog sampler, binding 1 = fog params UBO
    VkDescriptorSetLayoutBinding b[2]{};
    b[0].binding = 0;
    b[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    b[0].descriptorCount = 1;
    b[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    b[1].binding = 1;
    b[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    b[1].descriptorCount = 1;
    b[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo li{};
    li.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    li.bindingCount = 2;
    li.pBindings = b;
    vkCreateDescriptorSetLayout(device, &li, nullptr, &m_SetLayout);

    VkDescriptorPoolSize ps[2]{
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }
    };
    VkDescriptorPoolCreateInfo pi{};
    pi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pi.poolSizeCount = 2;
    pi.pPoolSizes = ps;
    pi.maxSets = 1;
    vkCreateDescriptorPool(device, &pi, nullptr, &m_Pool);

    VkDescriptorSetAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool = m_Pool;
    ai.descriptorSetCount = 1;
    ai.pSetLayouts = &m_SetLayout;
    vkAllocateDescriptorSets(device, &ai, &m_Set);

    VkDescriptorImageInfo img{};
    img.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    img.imageView = fogTex->GetView();
    img.sampler = fogTex->GetSampler();
    VkDescriptorBufferInfo buf{ fogTex->GetParamsBuffer(), 0, sizeof(glm::vec4) };

    VkWriteDescriptorSet w[2]{};
    w[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = m_Set;
    w[0].dstBinding = 0;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w[0].descriptorCount = 1;
    w[0].pImageInfo = &img;
    w[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[1].dstSet = m_Set;
    w[1].dstBinding = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    w[1].descriptorCount = 1;
    w[1].pBufferInfo = &buf;
    vkUpdateDescriptorSets(device, 2, w, 0, nullptr);

    // Pipeline layout: one set, push constants (mat4 + vec4 = 80 bytes) for vertex and fragment
    VkPushConstantRange pr{ VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 80 };
    VkPipelineLayoutCreateInfo pli{};
    pli.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pli.setLayoutCount = 1;
    pli.pSetLayouts = &m_SetLayout;
    pli.pushConstantRangeCount = 1;
    pli.pPushConstantRanges = &pr;
    vkCreatePipelineLayout(device, &pli, nullptr, &m_Layout);

    auto vertCode = m_Engine->ReadFile("Shaders/fog_layer_vert.spv");
    auto fragCode = m_Engine->ReadFile("Shaders/fog_layer_frag.spv");
    VkShaderModule vm = m_Engine->CreateShaderModule(vertCode);
    VkShaderModule fm = m_Engine->CreateShaderModule(fragCode);

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vm;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fm;
    stages[1].pName = "main";

    VkPipelineVertexInputStateCreateInfo vin{}; // empty: the vertex shader generates the quad
    vin.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo ia{};
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

    VkPipelineViewportStateCreateInfo vp{};
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.scissorCount = 1;

    VkDynamicState dyn[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo ds{};
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    ds.dynamicStateCount = 2;
    ds.pDynamicStates = dyn;

    VkPipelineRasterizationStateCreateInfo rs{};
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo dss{};
    dss.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    dss.depthTestEnable = VK_TRUE;
    dss.depthWriteEnable = VK_FALSE; // clouds don't write depth
    dss.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState cba{};
    cba.blendEnable = VK_TRUE;
    cba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    cba.colorBlendOp = VK_BLEND_OP_ADD;
    cba.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    cba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    cba.alphaBlendOp = VK_BLEND_OP_ADD;
    cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo cb{};
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.attachmentCount = 1;
    cb.pAttachments = &cba;

    VkFormat colorFormat = m_Engine->GetSwapChainFormat();
    VkPipelineRenderingCreateInfo ri{};
    ri.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    ri.colorAttachmentCount = 1;
    ri.pColorAttachmentFormats = &colorFormat;
    ri.depthAttachmentFormat = m_Engine->GetDepthFormat();

    VkGraphicsPipelineCreateInfo gi{};
    gi.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gi.pNext = &ri;
    gi.stageCount = 2;
    gi.pStages = stages;
    gi.pVertexInputState = &vin;
    gi.pInputAssemblyState = &ia;
    gi.pViewportState = &vp;
    gi.pRasterizationState = &rs;
    gi.pMultisampleState = &ms;
    gi.pDepthStencilState = &dss;
    gi.pColorBlendState = &cb;
    gi.pDynamicState = &ds;
    gi.layout = m_Layout;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gi, nullptr, &m_Pipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create fog layer pipeline");

    vkDestroyShaderModule(device, fm, nullptr);
    vkDestroyShaderModule(device, vm, nullptr);
} // end of Init

void FogLayerRenderer::Record(VkCommandBuffer cmd, const glm::mat4 &viewProj) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Layout, 0, 1, &m_Set, 0, nullptr);

    VkExtent2D extent = m_Engine->GetSwapChainExtent();
    VkViewport viewport{ 0, 0, (float)extent.width, (float)extent.height, 0, 1 };
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    VkRect2D scissor{ { 0, 0 }, extent };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    const float heights[3] = { 4.0f, 9.0f, 15.0f }; // tune to your terrain's height range
    for (int i = 0; i < 3; i++) {
        struct {
            glm::mat4 vp;
            glm::vec4 layer;
        } pc{ viewProj, glm::vec4(heights[i], 0.0f, (float)i, 0.0f) };
        vkCmdPushConstants(cmd, m_Layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(pc), &pc);
        vkCmdDraw(cmd, 4, 1, 0, 0);
    }
}
void FogLayerRenderer::Shutdown() {
    VkDevice device = m_Engine->GetDevice();
    if (m_Pipeline) vkDestroyPipeline(device, m_Pipeline, nullptr);
    if (m_Layout) vkDestroyPipelineLayout(device, m_Layout, nullptr);
    if (m_Pool) vkDestroyDescriptorPool(device, m_Pool, nullptr); // also frees m_Set
    if (m_SetLayout) vkDestroyDescriptorSetLayout(device, m_SetLayout, nullptr);
    m_Pipeline = VK_NULL_HANDLE;
    m_Layout = VK_NULL_HANDLE;
    m_Pool = VK_NULL_HANDLE;
    m_SetLayout = VK_NULL_HANDLE;
}