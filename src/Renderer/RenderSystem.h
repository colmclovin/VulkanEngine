#pragma once
#include <memory>
#include <entt/entt.hpp>
#include "../Game/ItemDatabase.h"
#include "../Game/TechState.h"
#include "../Helpers/PauseMenuAction.h"
#include "../World/DayNightCycle.h"
#include "../Game/ChunkManager.h"
#include "../Rendering/Mesh.h"
#include <functional>


class VulkanEngine;
class QuadRenderer;
class SkinnedMeshRenderer;
class ShadowMap;
class ShadowMapRenderer;
class ChunkGenerator;
class ImGuiVulkanUtil;
class AudioEngine;
//class ResourceManager;
class TreeRenderer;
class FogLayerRenderer;
class MeshRenderer;
class DebugLineRenderer;
class TerrainRenderer;
class Camera3D;
class DebugUI;
struct GameSettings;
class FogTexture;

class RenderSystem {

public:
    RenderSystem(VulkanEngine *engine);
    ~RenderSystem();
    
    
    void Init(uint32_t terrainVertsPerChunk, uint32_t terrainIndicesPerChunk, uint32_t terrainMaxChunks);
    PauseMenuAction RenderFrame(entt::registry &registry, Camera3D &camera, GameSettings &settings, AudioEngine &audioEngine, entt::entity m_PlayerEntity, entt::entity m_InspectedEntity, ItemId &selectedItem, TechState &techState, bool isPaused, bool &showOptionsInPause, DayNightCycle& dayNightCycle, ChunkManager& chunkManager, DepletionMap& depletionMap, RemovedTreesMap& removedTreesMap);
    void Shutdown();

    //ResourceManager *GetResourceManager() const { return m_ResourceManager.get(); }
    MeshRenderer *GetMeshRenderer() const { return m_MeshRenderer.get(); }
    SkinnedMeshRenderer *GetSkinnedMeshRenderer() const { return m_SkinnedMeshRenderer.get(); }
    ShadowMapRenderer* GetShadowMapRenderer() const { return m_ShadowMapRenderer.get(); }
    QuadRenderer *GetQuadRenderer() const { return m_QuadRenderer.get(); }
    DebugLineRenderer *GetDebugLineRenderer() const { return m_DebugLineRenderer.get(); }
    ChunkGenerator* GetChunkGenerator() const { return m_ChunkGenerator.get(); }
    TerrainRenderer* GetTerrainRenderer() const { return m_TerrainRenderer.get(); }
    FogLayerRenderer *GetFogLayerRenderer() const { return m_FogLayerRenderer.get(); };
    TreeRenderer* GetTreeRenderer() const { return m_TreeRenderer.get(); }
    DebugUI* GetDebugUI() const;
    ImGuiVulkanUtil *GetImGuiUtil() const;
    void SetSelectionRect(bool active, glm::vec2 a, glm::vec2 b) { m_SelActive = active; m_SelA = a; m_SelB = b; }
    FogTexture *GetFogTexture() const { return m_FogTexture.get(); }
    //void SetUICallback(std::function<void()> cb) { m_UICallback = std::move(cb); }
    void SetPreRenderCallback(std::function<void(VkCommandBuffer)> cb) { m_PreRenderCallback = std::move(cb); }
    void SetUICallback(std::function<void()> cb) { m_UICallback = std::move(cb); }
private:
    VulkanEngine *m_Engine = nullptr;
    Mesh m_TreeMesh;
    //std::function<void()> m_UICallback;
    std::function<void(VkCommandBuffer)> m_PreRenderCallback;
    std::function<void()> m_UICallback;
    // Rendering subsystems
    //std::unique_ptr<ResourceManager> m_ResourceManager;
    std::unique_ptr<MeshRenderer> m_MeshRenderer;
    std::unique_ptr<SkinnedMeshRenderer> m_SkinnedMeshRenderer;
    std::unique_ptr<ChunkGenerator> m_ChunkGenerator;
    std::unique_ptr<ShadowMap> m_ShadowMap;
    std::unique_ptr<ShadowMapRenderer> m_ShadowMapRenderer;
    std::unique_ptr<QuadRenderer> m_QuadRenderer;
    std::unique_ptr<TerrainRenderer> m_TerrainRenderer;
    std::unique_ptr<TreeRenderer> m_TreeRenderer;
    std::unique_ptr<DebugLineRenderer> m_DebugLineRenderer;
	std::unique_ptr<ImGuiVulkanUtil> m_ImGuiVulkanUtil;
    std::unique_ptr<FogLayerRenderer> m_FogLayerRenderer;
    std::unique_ptr<FogTexture> m_FogTexture;
    std::unique_ptr<DebugUI> m_DebugUI;
    bool m_initialized = false;
    bool m_SelActive = false;
    glm::vec2 m_SelA{}, m_SelB{};
};