#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <entt/entt.hpp>
#include "../Game/Camera3D.h"
#include "../Components/Texture.h"
#include "../Components/DayNightCycle.h"
#include "ShadowMap.h"
class VulkanEngine;
class Mesh;


class MeshRenderer {

public:
    MeshRenderer(VulkanEngine* m_Engine);
    ~MeshRenderer();
    void Init(ShadowMap* shadowMap);
    void Render(entt::registry& registry, const Camera3D& camera, bool wireframe, DayNightCycle& dayNightCycle, const glm::mat4& lightSpaceMatrix);
    void Shutdown();

    VkDescriptorSetLayout GetTextureDescriptorSetLayout() const { return m_TextureDescriptorSetLayout; }
    VkDescriptorSet AllocateTextureDescriptorSet(VkImageView imageView, VkSampler sampler);

private:

    void CreateUniformBuffers();
    void CreatePipeline();
    std::shared_ptr<Texture> m_DefaultTexture;
    VkDescriptorSet m_DefaultDescriptorSet = VK_NULL_HANDLE;

    VulkanEngine* m_Engine = nullptr;
    // Pipeline and layout
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;            // fill mode
    VkPipeline m_WireframePipeline = VK_NULL_HANDLE;   // line mode

    VkDescriptorSetLayout m_TextureDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_TextureDescriptorPool = VK_NULL_HANDLE;
    VkSampler m_DefaultSampler = VK_NULL_HANDLE; // fallback for materials with no texture

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2; // match your existing engine's frame count
    VkDescriptorSetLayout m_LightingDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_LightingDescriptorPool = VK_NULL_HANDLE;
    VkBuffer m_LightingUBOBuffers[MAX_FRAMES_IN_FLIGHT];
    VkDeviceMemory m_LightingUBOMemory[MAX_FRAMES_IN_FLIGHT];
    void* m_LightingUBOMapped[MAX_FRAMES_IN_FLIGHT];
    VkDescriptorSet m_LightingDescriptorSets[MAX_FRAMES_IN_FLIGHT];
    ShadowMap* m_ShadowMap = nullptr;

    VkFormat m_SwapChainImageFormat = VK_FORMAT_UNDEFINED;

    // Shared Mesh for all models
    Mesh* m_MeshMesh = nullptr;
    bool m_MeshUploaded = false;

    struct MeshPushConstants {
        glm::mat4 model;
        glm::vec4 baseColor;
    };
    bool m_initialized = false;

};