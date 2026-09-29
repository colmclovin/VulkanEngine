// TerrainRenderer.h
#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <entt/entt.hpp>
#include "../Rendering/TerrainBufferPool.h"
#include "../Rendering/ChunkCuller.h"
#include "../Rendering/Frustum.h"
#include "../Renderer/ShadowMap.h"

class VulkanEngine;
class Camera3D;
class DayNightCycle;
struct PointLight;

class TerrainRenderer {
public:
    TerrainRenderer(VulkanEngine* engine);
    void Init(uint32_t vertsPerChunk, uint32_t indicesPerChunk, uint32_t maxChunks, ShadowMap* shadowMap);
    void Shutdown();
    TerrainBufferPool* GetBufferPool() { return &m_Pool; }
    // Called by ChunkManager when a chunk finishes generating
    std::optional<uint32_t> AllocateChunkSlot();
    void UploadChunk(uint32_t slot, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices,
        glm::vec3 boundsCenter, float boundsRadius);
    void FreeChunkSlot(uint32_t slot);
    void ProcessPendingFrees() {
        m_Pool.ProcessPendingFrees();
    }
    void UploadChunkBounds(uint32_t slot, glm::vec3 boundsCenter, float boundsRadius);
    void Render(VkCommandBuffer commandBuffer, const Camera3D& camera, DayNightCycle& dayNightCycle,
        const glm::mat4& lightSpaceMatrix, const std::vector<PointLight>& activeLights, bool wireframe);
    void RecordCullingPass(VkCommandBuffer commandBuffer, const Frustum& frustum);
    void RecordDraw(VkCommandBuffer commandBuffer, const Camera3D& camera, DayNightCycle& dayNightCycle,
        const glm::mat4& lightSpaceMatrix, const std::vector<PointLight>& activeLights, bool wireframe);
private:
    void CreateGraphicsPipeline();

    VulkanEngine* m_Engine;
    TerrainBufferPool m_Pool;
    ChunkCuller m_Culler;

    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipeline m_WireframePipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_LightingDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_LightingDescriptorPool = VK_NULL_HANDLE;

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    VkBuffer m_LightingUBOBuffers[MAX_FRAMES_IN_FLIGHT];
    VkDeviceMemory m_LightingUBOMemory[MAX_FRAMES_IN_FLIGHT];
    void* m_LightingUBOMapped[MAX_FRAMES_IN_FLIGHT];
    VkDescriptorSet m_LightingDescriptorSets[MAX_FRAMES_IN_FLIGHT];

    uint32_t m_MaxChunks = 0;
    uint32_t m_VertsPerChunk = 0;
    uint32_t m_IndicesPerChunk = 0;
};