#pragma once
#include <memory>
#include <entt/entt.hpp>
#include "../Game/ItemDatabase.h"
#include "../Game/TechState.h"
#include "../Helpers/PauseMenuAction.h"
#include "../World/DayNightCycle.h"
#include "../Game/ChunkManager.h"
#include "../Rendering/Mesh.h"

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
class MeshRenderer;
class DebugLineRenderer;
class TerrainRenderer;
class Camera3D;
class DebugUI;
struct GameSettings;

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
    TreeRenderer* GetTreeRenderer() const { return m_TreeRenderer.get(); }
    DebugUI* GetDebugUI() const;
    ImGuiVulkanUtil *GetImGuiUtil() const;

private:
    VulkanEngine *m_Engine = nullptr;
    Mesh m_TreeMesh;
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
    std::unique_ptr<DebugUI> m_DebugUI;
    bool m_initialized = false;
};