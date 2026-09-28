// ShadowMapRenderer.h
#pragma once
#include <vulkan/vulkan.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include "../Rendering/Mesh.h"
class VulkanEngine;
class ShadowMap;


class Mesh;


class ShadowMapRenderer {
public:
    ShadowMapRenderer(VulkanEngine* engine);
    void Init();
    void BeginShadowPass(ShadowMap &shadowMap);
    void RenderStatic(entt::registry &registry, const glm::mat4 &lightSpaceMatrix);
    void EndShadowPass(ShadowMap &shadowMap);
    void Shutdown();
    struct ShadowInstanceData {
        glm::mat4 model;
    };

private:
    void CreatePipeline();

    VulkanEngine* m_Engine;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;


    void CreateInstancedPipeline();
    void EnsureInstanceBufferCapacity(size_t instanceCount);
    void DrawInstancedShadowGroup(entt::registry& registry, Mesh* mesh, const std::vector<entt::entity>& entities,
        VkCommandBuffer commandBuffer, const glm::mat4& lightSpaceMatrix);

    static VkVertexInputBindingDescription GetInstanceBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 4> GetInstanceAttributeDescriptions();

    VkPipeline m_InstancedPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_InstancedPipelineLayout = VK_NULL_HANDLE;

    VkBuffer m_InstanceBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_InstanceBufferMemory = VK_NULL_HANDLE;
    void* m_InstanceBufferMapped = nullptr;
    size_t m_InstanceBufferCapacity = 0;
    size_t m_InstanceBufferWriteOffset = 0;


};