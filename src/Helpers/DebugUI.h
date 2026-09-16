// DebugUI.h
#pragma once
#include <entt/entt.hpp>
#include "../Game/ItemDatabase.h"

class RenderSystem;
class Camera3D;
class AudioEngine;
struct GameSettings;


class DebugUI {
public:
    void Draw(entt::registry &registry, RenderSystem *renderSystem, Camera3D *camera, GameSettings &settings, AudioEngine *audioEngine, entt::entity m_PlayerEntity, entt::entity m_InspectedEntity, ItemId &selectedItem);

private:
    void DrawStats(entt::registry &registry);
    void DrawCrafting(entt::registry& registry, entt::entity player);
    void DrawEntityList(entt::registry& registry);
    void DrawSettingsTab(Camera3D* camera, GameSettings& settings, AudioEngine* audioEngine);
    void DrawInventory(entt::registry &registry, entt::entity player, ItemId &selectedItem); 
    bool m_ShowDemo = false;
    bool m_RegenerateTerrainRequested = false;   // set true when user clicks "Regenerate"
    void DrawMachineInspector(entt::registry &registry, entt::entity target);

public:
    bool ConsumeRegenerateRequest() {
        bool r = m_RegenerateTerrainRequested;
        m_RegenerateTerrainRequested = false;
        return r;
    }
};