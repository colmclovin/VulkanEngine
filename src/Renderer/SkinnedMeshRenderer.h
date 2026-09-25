#pragma once
#include "../Components/BoneMatrixUBO.h"
#include <vulkan/vulkan.h>
#include <entt/entt.hpp>
#include <memory>
#include "../Components/DayNightCycle.h"

class VulkanEngine;
class Camera3D;
class Texture;

class SkinnedMeshRenderer {
public:
    SkinnedMeshRenderer(VulkanEngine *engine);
    void Init();
    void Render(entt::registry &registry, const Camera3D &camera, bool wireframe, DayNightCycle& dayNightCycle);
    void Shutdown();

private:
    void CreatePipeline();
    void CreateUniformBuffers();
    VkDescriptorSet AllocateTextureDescriptorSet(VkImageView imageView, VkSampler sampler);

    VulkanEngine *m_Engine;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_WireframePipeline = VK_NULL_HANDLE;   // line mode


    VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE; // binding 0 = UBO, binding 1 = sampler
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;

    // One UBO per frame-in-flight, since bone data changes every frame
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2; // match your existing engine's frame count
    VkBuffer m_BoneUBOBuffers[MAX_FRAMES_IN_FLIGHT];
    VkDeviceMemory m_BoneUBOMemory[MAX_FRAMES_IN_FLIGHT];
    void *m_BoneUBOMapped[MAX_FRAMES_IN_FLIGHT];
    VkDescriptorSet m_DescriptorSets[MAX_FRAMES_IN_FLIGHT]; // one combining UBO + default texture, per frame

    VkBuffer m_LightingUBOBuffers[MAX_FRAMES_IN_FLIGHT];
    VkDeviceMemory m_LightingUBOMemory[MAX_FRAMES_IN_FLIGHT];
    void* m_LightingUBOMapped[MAX_FRAMES_IN_FLIGHT];

    bool m_initialized = false;


    std::shared_ptr<Texture> m_DefaultTexture;
};