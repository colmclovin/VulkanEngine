#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <entt/entt.hpp>
#include "../Game/Camera3D.h"
#include "../Components/Texture.h"


class VulkanEngine;
class Mesh;


class MeshRenderer {

public:
    MeshRenderer(VulkanEngine* m_Engine);
    ~MeshRenderer();
    void Init();
    void Render(entt::registry& registry, const Camera3D& camera, bool wireframe);
    void Shutdown();

    VkDescriptorSetLayout GetTextureDescriptorSetLayout() const { return m_TextureDescriptorSetLayout; }
    VkDescriptorSet AllocateTextureDescriptorSet(VkImageView imageView, VkSampler sampler);

private:


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


    VkFormat m_SwapChainImageFormat = VK_FORMAT_UNDEFINED;

    // Shared Mesh for all models
    Mesh* m_MeshMesh = nullptr;
    bool m_MeshUploaded = false;

    struct MeshPushConstants {
        glm::mat4 mvp;
        glm::vec4 baseColor;   // NEW
    };
    bool m_initialized = false;

};