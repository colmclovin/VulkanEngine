#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <optional>
#include "../Rendering/TreeInstanceBufferPool.h"
#include "../Rendering/TreeCuller.h"
#include "../Rendering/Frustum.h"
#include "../Renderer/ShadowMap.h"

class VulkanEngine;
class Camera3D;
class DayNightCycle;
class ShadowMap;
struct GpuDrawCommand;
struct PointLight;
struct TreeSubmesh {
    uint32_t indexCount;
    uint32_t firstIndex;
    VkImageView textureView;
    VkSampler textureSampler;
};
struct TreeSubmeshRenderInfo {
    uint32_t indexCount;
    uint32_t firstIndex;
    VkImageView textureView;
    VkSampler textureSampler;
};

class TreeRenderer {
public:
    TreeRenderer(VulkanEngine* engine);
    void Init(uint32_t maxTreesPerChunk, uint32_t maxChunks, ShadowMap* shadowMap,
        VkBuffer treeVertexBuffer, VkBuffer treeIndexBuffer, const std::vector<TreeSubmeshRenderInfo>& submeshes,
        VkImageView defaultTextureView, VkSampler defaultTextureSampler);
    void Shutdown();
    TreeInstanceBufferPool* GetBufferPool() { return &m_Pool; }
    std::optional<uint32_t> AllocateChunkSlot();
    void FreeChunkSlot(uint32_t slot);
    void ProcessPendingFrees();
    void UploadChunkBounds(uint32_t slot, glm::vec3 boundsCenter, float boundsRadius);

    void RecordCullingPass(VkCommandBuffer commandBuffer, const Frustum& frustum);
    void RecordDraw(VkCommandBuffer commandBuffer, const Camera3D& camera, DayNightCycle& dayNightCycle,
        const glm::mat4& lightSpaceMatrix, const std::vector<PointLight>& activeLights);
    std::vector<TreeInstanceReadback> ReadBackChunkInstances(uint32_t slot) const { return m_Pool.ReadBackChunkInstances(slot); }
    void RecordShadowPass(VkCommandBuffer commandBuffer, const glm::mat4& lightSpaceMatrix);

private:
    void CreateGraphicsPipeline();
    void CreateShadowPipeline();
    VkPipeline m_ShadowPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_ShadowPipelineLayout = VK_NULL_HANDLE;
    VulkanEngine* m_Engine;
    TreeInstanceBufferPool m_Pool;
    TreeCuller m_Culler;

    VkBuffer m_TreeVertexBuffer = VK_NULL_HANDLE;   // the actual tree MESH — shared by every instance, NOT per-chunk generated
    VkBuffer m_TreeIndexBuffer = VK_NULL_HANDLE;
    uint32_t m_TreeIndexCount = 0;
    VkImageView m_DefaultTextureView = VK_NULL_HANDLE;
    VkSampler m_DefaultTextureSampler = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_LightingDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_LightingDescriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> m_SubmeshTextureDescSets;
    VkDescriptorSetLayout m_TextureDescSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_TextureDescPool = VK_NULL_HANDLE;
    std::vector<TreeSubmeshRenderInfo> m_SubmeshRenderInfo;
    VkDescriptorSetLayout m_InstanceDescSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_InstanceDescPool = VK_NULL_HANDLE;
    VkDescriptorSet m_InstanceDescSet = VK_NULL_HANDLE;
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    VkBuffer m_LightingUBOBuffers[MAX_FRAMES_IN_FLIGHT];
    VkDeviceMemory m_LightingUBOMemory[MAX_FRAMES_IN_FLIGHT];
    void* m_LightingUBOMapped[MAX_FRAMES_IN_FLIGHT];
    VkDescriptorSet m_LightingDescriptorSets[MAX_FRAMES_IN_FLIGHT];

    uint32_t m_MaxChunks = 0;
};