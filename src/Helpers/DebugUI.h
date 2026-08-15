// DebugUI.h
#pragma once
#include <entt/entt.hpp>

class RenderSystem;
class Camera3D;
class AudioEngine;
struct GameSettings;

class DebugUI {
public:
    void Draw(entt::registry &registry, RenderSystem *renderSystem, Camera3D *camera, GameSettings &settings, AudioEngine *audioEngine, entt::entity m_PlayerEntity);

    private:
    void DrawStats();
    void DrawEntityList(entt::registry& registry);
    void DrawSettingsTab(Camera3D* camera, GameSettings& settings, AudioEngine* audioEngine);
    void DrawInventory(entt::registry &registry, entt::entity player); 
    bool m_ShowDemo = false;
    bool m_RegenerateTerrainRequested = false;   // set true when user clicks "Regenerate"

public:
    bool ConsumeRegenerateRequest() {
        bool r = m_RegenerateTerrainRequested;
        m_RegenerateTerrainRequested = false;
        return r;
    }
};