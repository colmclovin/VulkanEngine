#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <entt/entt.hpp>
#include "../Game/Camera3D.h"
#include "../Rendering/Texture.h"
#include "../World/DayNightCycle.h"
#include "ShadowMap.h"
#include "../Components/PointLightComponent.h"
#include "../Rendering/LightingUBO.h"
class VulkanEngine;
class Mesh;


class MeshRenderer {

public:
    MeshRenderer(VulkanEngine* m_Engine);
    ~MeshRenderer();
    void Init(ShadowMap* shadowMap);
    void Render(entt::registry &registry, const Camera3D &camera, bool wireframe, DayNightCycle &dayNightCycle, const glm::mat4 &lightSpaceMatrix, const std::vector<PointLight> &activeLights);
    void Shutdown();
    VkImageView GetDefaultTextureView() const { return m_DefaultTexture->imageView; }
    VkSampler GetDefaultTextureSampler() const { return m_DefaultTexture->sampler; }
    VkDescriptorSetLayout GetTextureDescriptorSetLayout() const { return m_TextureDescriptorSetLayout; }
    VkDescriptorSet AllocateTextureDescriptorSet(VkImageView imageView, VkSampler sampler);
    struct InstanceData {
        glm::mat4 model;
        glm::vec4 tintColor;
    };

private:
    void DrawSingleEntity(entt::registry &registry, entt::entity entity, VkCommandBuffer commandBuffer,
                          const glm::mat4 &view, const glm::mat4 &proj);
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

    static VkVertexInputBindingDescription GetInstanceBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 5> GetInstanceAttributeDescriptions(); // mat4 = 4 vec4 slots + 1 tint

    void CreateInstancedPipeline();
    void EnsureInstanceBufferCapacity(size_t instanceCount);
    void DrawInstancedGroup(entt::registry &registry, Mesh *mesh, const std::vector<entt::entity> &entities,
                            VkCommandBuffer commandBuffer);

    VkPipeline m_InstancedPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_InstancedPipelineLayout = VK_NULL_HANDLE;

    VkBuffer m_InstanceBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_InstanceBufferMemory = VK_NULL_HANDLE;
    void *m_InstanceBufferMapped = nullptr;
    size_t m_InstanceBufferCapacity = 0; // in number of InstanceData elements

    static constexpr size_t INSTANCING_THRESHOLD = 4;
    size_t m_InstanceBufferWriteOffset = 0; // reset to 0 at the start of each Render() call
    // Shared Mesh for all models
    Mesh* m_MeshMesh = nullptr;
    bool m_MeshUploaded = false;

    struct MeshPushConstants {
        glm::mat4 model;
        glm::vec4 baseColor;
    };
    bool m_initialized = false;

};