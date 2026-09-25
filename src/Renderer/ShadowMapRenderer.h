// ShadowMapRenderer.h
#pragma once
#include <vulkan/vulkan.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>

class VulkanEngine;
class ShadowMap;

class ShadowMapRenderer {
public:
    ShadowMapRenderer(VulkanEngine* engine);
    void Init();
    void Render(entt::registry& registry, ShadowMap& shadowMap, const glm::mat4& lightSpaceMatrix);
    void Shutdown();

private:
    void CreatePipeline();

    VulkanEngine* m_Engine;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
};