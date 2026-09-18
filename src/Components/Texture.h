#pragma once
#include <vulkan/vulkan.h>

class VulkanEngine;
struct TextureData;

class Texture {
public:
    void UploadToGPU(VulkanEngine *engine, const TextureData &data);
    void DestroyGPUResources(VkDevice device);

    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;

private:
    VkImage m_Image = VK_NULL_HANDLE;
    VkDeviceMemory m_ImageMemory = VK_NULL_HANDLE;
};