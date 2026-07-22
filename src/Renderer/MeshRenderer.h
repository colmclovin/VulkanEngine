#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <entt/entt.hpp>
#include "../Game/Camera3D.h"

class VulkanEngine;
class Mesh;


class MeshRenderer {

public:
    MeshRenderer(VulkanEngine* m_Engine);
    ~MeshRenderer();
    void Init();
    void Render(entt::registry& registry, const Camera3D& camera);
    void Shutdown();
private:


    void CreatePipeline();


    VulkanEngine* m_Engine = nullptr;
    // Pipeline and layout
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;


    VkFormat m_SwapChainImageFormat = VK_FORMAT_UNDEFINED;

    // Shared Mesh for all models
    Mesh* m_MeshMesh = nullptr;
    bool m_MeshUploaded = false;

    struct MeshPushConstants {
        glm::mat4 mvp;
    };
    bool m_initialized = false;

};