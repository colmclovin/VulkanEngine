#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <entt/entt.hpp>

class VulkanEngine;
class Mesh;


class QuadRenderer {

public:
    QuadRenderer(VulkanEngine *m_Engine);
    ~QuadRenderer();
    void Init();
    void Render(entt::registry &registry);
    void Shutdown();
private:


    void CreateQuad();
    void CreatePipeline();
    
    
    VulkanEngine *m_Engine = nullptr;
    // Pipeline and layout
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    // Shared quad mesh for all sprites
    Mesh *m_QuadMesh = nullptr;
    bool m_QuadUploaded = false;

    struct SpritePushConstants {
        alignas(16) glm::mat4 transform; // Position, rotation, scale in screen space
        alignas(16) glm::vec4 color; // Sprite tint color
    };
    
};