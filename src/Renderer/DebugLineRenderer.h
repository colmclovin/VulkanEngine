// DebugLineRenderer.h
#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>

class VulkanEngine;
class Camera3D;

struct DebugLineVertex {
    glm::vec3 position;
    glm::vec3 color;
};

class DebugLineRenderer {
public:
    DebugLineRenderer(VulkanEngine *engine);
    void Init();
    void Shutdown();

    void BeginFrame(); // clears the line buffer for this frame
    void AddLine(glm::vec3 a, glm::vec3 b, glm::vec3 color);
    void AddBox(glm::vec3 min, glm::vec3 max, glm::vec3 color); // convenience: 12 lines forming a box
    void Render(const Camera3D &camera, float aspect);

private:
    void CreatePipeline();

    VulkanEngine *m_Engine;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    std::vector<DebugLineVertex> m_Lines;
    VkBuffer m_VertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_VertexBufferMemory = VK_NULL_HANDLE;
    size_t m_BufferCapacity = 0;
};