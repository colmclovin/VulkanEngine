#pragma once
#include <vulkan/vulkan.h>

class VulkanEngine;

class ShadowMap {
public:
    void Create(VulkanEngine* engine, uint32_t resolution = 2048);
    void Destroy(VkDevice device);

    VkImageView GetImageView() const { return m_ImageView; }
    VkImage GetImage() const { return m_Image; }
    VkSampler GetSampler() const { return m_Sampler; }
    VkFormat GetFormat() const { return m_Format; }
    uint32_t GetResolution() const { return m_Resolution; }

	

private:
    VkImage m_Image = VK_NULL_HANDLE;
    VkDeviceMemory m_ImageMemory = VK_NULL_HANDLE;
    VkImageView m_ImageView = VK_NULL_HANDLE;
    VkSampler m_Sampler = VK_NULL_HANDLE;
    VkFormat m_Format = VK_FORMAT_D32_SFLOAT;
    uint32_t m_Resolution = 2048;
};
