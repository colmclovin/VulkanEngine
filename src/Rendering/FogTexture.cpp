// FogTexture.cpp
#include "FogTexture.h"


void FogTexture::Init(VulkanEngine *engine) {
    m_Engine = engine;
    VkDevice dev = engine->GetDevice();

    VkImageCreateInfo ii{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    ii.imageType = VK_IMAGE_TYPE_2D;
    ii.format = VK_FORMAT_R8_UNORM;
    ii.extent = { SIZE, SIZE, 1 };
    ii.mipLevels = 1;
    ii.arrayLayers = 1;
    ii.samples = VK_SAMPLE_COUNT_1_BIT;
    ii.tiling = VK_IMAGE_TILING_OPTIMAL;
    ii.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vkCreateImage(dev, &ii, nullptr, &m_Image);

    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(dev, m_Image, &req);
    VkMemoryAllocateInfo ai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    ai.allocationSize = req.size;
    ai.memoryTypeIndex = engine->FindMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); // adapt name
    vkAllocateMemory(dev, &ai, nullptr, &m_Mem);
    vkBindImageMemory(dev, m_Image, m_Mem, 0);

    VkImageViewCreateInfo vi{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    vi.image = m_Image;
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format = VK_FORMAT_R8_UNORM;
    vi.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    vkCreateImageView(dev, &vi, nullptr, &m_View);

    VkSamplerCreateInfo si{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    si.magFilter = si.minFilter = VK_FILTER_LINEAR;
    si.addressModeU = si.addressModeV = si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    si.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK; // outside the window = unexplored
    vkCreateSampler(dev, &si, nullptr, &m_Sampler);

    VkDeviceSize bytes = SIZE * SIZE;
    engine->CreateBuffer(bytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_Staging, m_StagingMem);
    vkMapMemory(dev, m_StagingMem, 0, bytes, 0, reinterpret_cast<void **>(&m_Mapped));
    memset(m_Mapped, 0, bytes);


    engine->CreateBuffer(sizeof(glm::vec4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_ParamsBuf, m_ParamsMem);
    vkMapMemory(dev, m_ParamsMem, 0, sizeof(glm::vec4), 0, reinterpret_cast<void **>(&m_ParamsMapped));

    Update(FogOfWar{}, glm::vec3(0)); // puts the image in SHADER_READ_ONLY before first use
}

void FogTexture::Update(const FogOfWar &fog, glm::vec3 c) {
    const float world = SIZE * FogOfWar::CELL;
    m_Origin.x = std::floor((c.x - world * 0.5f) / FogOfWar::CELL) * FogOfWar::CELL;
    m_Origin.y = std::floor((c.z - world * 0.5f) / FogOfWar::CELL) * FogOfWar::CELL;
    fog.FillWindow(m_Mapped, m_Origin.x, m_Origin.y, SIZE);

    VkCommandBuffer cmd = m_Engine->BeginSingleTimeCommands();
    VkImageMemoryBarrier b{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    b.image = m_Image;
    b.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    b.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    b.srcAccessMask = 0;
    b.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);

    VkBufferImageCopy r{};
    r.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    r.imageExtent = { SIZE, SIZE, 1 };
    vkCmdCopyBufferToImage(cmd, m_Staging, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &r);

    b.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    b.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);
    m_Engine->EndSingleTimeCommands(cmd);

    *m_ParamsMapped = glm::vec4(m_Origin.x, m_Origin.y, GetInvWorldSize(), m_Time);
}

void FogTexture::SetTime(float t) {
    m_Time = t;
    *m_ParamsMapped = glm::vec4(m_Origin.x, m_Origin.y, GetInvWorldSize(), m_Time);
}
void FogTexture::Shutdown() {
    VkDevice device = m_Engine->GetDevice();
    vkUnmapMemory(device, m_StagingMem);
    vkUnmapMemory(device, m_ParamsMem);
    vkDestroyBuffer(device, m_Staging, nullptr);
    vkFreeMemory(device, m_StagingMem, nullptr);
    vkDestroyBuffer(device, m_ParamsBuf, nullptr);
    vkFreeMemory(device, m_ParamsMem, nullptr);
    vkDestroySampler(device, m_Sampler, nullptr);
    vkDestroyImageView(device, m_View, nullptr);
    vkDestroyImage(device, m_Image, nullptr);
    vkFreeMemory(device, m_Mem, nullptr);
}