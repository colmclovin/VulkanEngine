#include "Minimap.h"
#include "../Engine/VulkanEngine.h" // adjust path
#include "FogTexture.h"
#include "imgui_impl_vulkan.h"
#include <stdexcept>

struct MinimapPush {
    glm::vec2 center;
    float halfRange;
    float seed;
    glm::vec2 fogOrigin;
    float fogInvSize;
    float pad;
};

void Minimap::Init(VulkanEngine *engine, FogTexture *fogTex) {
    m_Engine = engine;
    m_FogTex = fogTex;
    VkDevice dev = engine->GetDevice();

    VkImageCreateInfo ii{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    ii.imageType = VK_IMAGE_TYPE_2D;
    ii.format = VK_FORMAT_R8G8B8A8_UNORM;
    ii.extent = { SIZE, SIZE, 1 };
    ii.mipLevels = 1;
    ii.arrayLayers = 1;
    ii.samples = VK_SAMPLE_COUNT_1_BIT;
    ii.tiling = VK_IMAGE_TILING_OPTIMAL;
    ii.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vkCreateImage(dev, &ii, nullptr, &m_Image);

    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(dev, m_Image, &req);
    VkMemoryAllocateInfo ai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    ai.allocationSize = req.size;
    ai.memoryTypeIndex = engine->FindMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); // same helper FogTexture used
    vkAllocateMemory(dev, &ai, nullptr, &m_Mem);
    vkBindImageMemory(dev, m_Image, m_Mem, 0);

    VkImageViewCreateInfo vi{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    vi.image = m_Image;
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format = VK_FORMAT_R8G8B8A8_UNORM;
    vi.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    vkCreateImageView(dev, &vi, nullptr, &m_View);

    VkSamplerCreateInfo si{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    si.magFilter = si.minFilter = VK_FILTER_LINEAR;
    si.addressModeU = si.addressModeV = si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    vkCreateSampler(dev, &si, nullptr, &m_Sampler);

    // The image stays in GENERAL for its whole life: compute writes it, ImGui reads it.
    // One-time transition from UNDEFINED.
    VkCommandBuffer cmd = engine->BeginSingleTimeCommands();
    VkImageMemoryBarrier b{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    b.image = m_Image;
    b.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    b.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &b);
    engine->EndSingleTimeCommands(cmd);

    m_ImGuiTex = ImGui_ImplVulkan_AddTexture(m_Sampler, m_View, VK_IMAGE_LAYOUT_GENERAL);

  VkDescriptorSetLayoutBinding lb[2]{};
    lb[0].binding = 0;
    lb[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    lb[0].descriptorCount = 1;
    lb[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    lb[1].binding = 1;
    lb[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    lb[1].descriptorCount = 1;
    lb[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo li{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    li.bindingCount = 2;
    li.pBindings = lb;
    vkCreateDescriptorSetLayout(dev, &li, nullptr, &m_SetLayout);

    VkDescriptorPoolSize ps[2]{
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
    };
    VkDescriptorPoolCreateInfo pi{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    pi.poolSizeCount = 2;
    pi.pPoolSizes = ps;
    pi.maxSets = 1;
    vkCreateDescriptorPool(dev, &pi, nullptr, &m_Pool);

    VkDescriptorSetAllocateInfo dai{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    dai.descriptorPool = m_Pool;
    dai.descriptorSetCount = 1;
    dai.pSetLayouts = &m_SetLayout;
    vkAllocateDescriptorSets(dev, &dai, &m_Set);

    VkDescriptorImageInfo outInfo{};
    outInfo.imageView = m_View;
    outInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkDescriptorImageInfo fogInfo{};
    fogInfo.sampler = fogTex->GetSampler();
    fogInfo.imageView = fogTex->GetView();
    fogInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet w[2]{};
    w[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = m_Set;
    w[0].dstBinding = 0;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    w[0].descriptorCount = 1;
    w[0].pImageInfo = &outInfo;
    w[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[1].dstSet = m_Set;
    w[1].dstBinding = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w[1].descriptorCount = 1;
    w[1].pImageInfo = &fogInfo;
    vkUpdateDescriptorSets(dev, 2, w, 0, nullptr);

    VkPushConstantRange pr{ VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(MinimapPush) };
    VkPipelineLayoutCreateInfo pli{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pli.setLayoutCount = 1;
    pli.pSetLayouts = &m_SetLayout;
    pli.pushConstantRangeCount = 1;
    pli.pPushConstantRanges = &pr;
    vkCreatePipelineLayout(dev, &pli, nullptr, &m_PipeLayout);

    auto code = engine->ReadFile("Shaders/minimap_comp.spv");
    VkShaderModule mod = engine->CreateShaderModule(code);
    VkPipelineShaderStageCreateInfo st{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    st.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    st.module = mod;
    st.pName = "main";
    VkComputePipelineCreateInfo ci{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
    ci.stage = st;
    ci.layout = m_PipeLayout;
    if (vkCreateComputePipelines(dev, VK_NULL_HANDLE, 1, &ci, nullptr, &m_Pipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create minimap pipeline");
    vkDestroyShaderModule(dev, mod, nullptr);
} // end of Init

void Minimap::Record(VkCommandBuffer cmd, glm::vec2 center, float halfRange, float seed) {
    // Wait for last frame's ImGui read before overwriting
    VkMemoryBarrier mb{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
    mb.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    mb.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         0, 1, &mb, 0, nullptr, 0, nullptr);

    MinimapPush push{};
    push.center = center;
    push.halfRange = halfRange;
    push.seed = seed;
    push.fogOrigin = m_FogTex->GetOrigin();
    push.fogInvSize = m_FogTex->GetInvWorldSize();

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipeLayout, 0, 1, &m_Set, 0, nullptr);
    vkCmdPushConstants(cmd, m_PipeLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);
    vkCmdDispatch(cmd, SIZE / 8, SIZE / 8, 1);

    // Make the writes visible to ImGui's fragment shader
    mb.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    mb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 1, &mb, 0, nullptr, 0, nullptr);
}

void Minimap::Shutdown() {
    VkDevice dev = m_Engine->GetDevice();
    vkDestroyPipeline(dev, m_Pipeline, nullptr);
    vkDestroyPipelineLayout(dev, m_PipeLayout, nullptr);
    vkDestroyDescriptorPool(dev, m_Pool, nullptr);
    vkDestroyDescriptorSetLayout(dev, m_SetLayout, nullptr);
    vkDestroySampler(dev, m_Sampler, nullptr);
    vkDestroyImageView(dev, m_View, nullptr);
    vkDestroyImage(dev, m_Image, nullptr);
    vkFreeMemory(dev, m_Mem, nullptr);
}