#pragma once
#include <memory>
#include <entt/entt.hpp>
#include "../Game/ItemDatabase.h"
#include "../Game/TechState.h"
#include "../Helpers/PauseMenuAction.h"
#include "../Components/DayNightCycle.h"

class VulkanEngine;
class QuadRenderer;
class SkinnedMeshRenderer;
class ImGuiVulkanUtil;
class AudioEngine;
//class ResourceManager;
class MeshRenderer;
class DebugLineRenderer;
class Camera3D;
class DebugUI;
struct GameSettings;

class RenderSystem {

public:
    RenderSystem(VulkanEngine *engine);
    ~RenderSystem();
    
    
    void Init();
    PauseMenuAction RenderFrame(entt::registry &registry, Camera3D &camera, GameSettings &settings, AudioEngine &audioEngine, entt::entity m_PlayerEntity, entt::entity m_InspectedEntity, ItemId &selectedItem, TechState &techState, bool isPaused, bool &showOptionsInPause, DayNightCycle& dayNightCycle);
    void Shutdown();

    //ResourceManager *GetResourceManager() const { return m_ResourceManager.get(); }
    MeshRenderer *GetMeshRenderer() const { return m_MeshRenderer.get(); }
    SkinnedMeshRenderer *GetSkinnedMeshRenderer() const { return m_SkinnedMeshRenderer.get(); }
    QuadRenderer *GetQuadRenderer() const { return m_QuadRenderer.get(); }
    DebugLineRenderer *GetDebugLineRenderer() const { return m_DebugLineRenderer.get(); }
    DebugUI* GetDebugUI() const;
    ImGuiVulkanUtil *GetImGuiUtil() const;

private:
    VulkanEngine *m_Engine = nullptr;

    // Rendering subsystems
    //std::unique_ptr<ResourceManager> m_ResourceManager;
    std::unique_ptr<MeshRenderer> m_MeshRenderer;
    std::unique_ptr<SkinnedMeshRenderer> m_SkinnedMeshRenderer;
    std::unique_ptr<QuadRenderer> m_QuadRenderer;
    std::unique_ptr<DebugLineRenderer> m_DebugLineRenderer;
	std::unique_ptr<ImGuiVulkanUtil> m_ImGuiVulkanUtil;
    std::unique_ptr<DebugUI> m_DebugUI;
    bool m_initialized = false;
};