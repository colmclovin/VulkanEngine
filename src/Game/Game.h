#pragma once
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>
#include "../Utils/GameSettings.h"
#include "../Audio/AudioEngine.h"
#include "../Audio/AudioEventSystem.h"
#include "../Helpers/InputState.h"
#include "../World/DayNightCycle.h"
#include "ItemDatabase.h"
#include "ChunkManager.h"
#include "WorldState.h"
#include "NestManager.h"
#include "GameHUD.h"

class VulkanEngine;
class RenderSystem;
class Camera3D;
class PlacementSystem;
class SkinnedMesh;
struct FrameContext;

enum class GameState {
    MainMenu,
    Playing,
    Paused,
    Loading,
};

class Game {
public:
    Game();
    ~Game();
    void Run();

private:
    // Cursor ray in physical (framebuffer) pixels, recomputed once per frame where needed
    struct MouseRay {
        glm::vec3 origin;
        glm::vec3 dir;
        glm::vec2 cursor;   // physical pixels
        glm::vec2 screen;   // swapchain extent
        float aspect;
    };

    // --- Lifecycle ---
    void Init();
    void Shutdown();
    void Update(float deltaTime);
    void Render();
    void DrawHUD();
    FrameContext MakeFrameContext();
    float GetAspect() const;

    // --- Input ---
    void HandleInput(float deltaTime);
    void HandleIsoInput(float deltaTime);
    void HandleRTSInput(const MouseRay &ray);
    void HandleFreeFlyInput(float deltaTime);
    void HandleInteract(entt::entity machineTarget, const MouseRay &ray);
    void HandleHotbarInput();
    void HandleDebugInput(const MouseRay &ray);
    MouseRay GetMouseRay() const;
    void MovePlayer(glm::vec3 direction, float deltaTime);
    void IssueMoveCommand(const glm::vec3 &groundTarget);
    void ClearUnitSelection();

    // --- World / saves ---
    void CreateInitialEntities();
    entt::entity SpawnUnit(ItemId unitItem, glm::vec3 position, uint8_t team);
    void ResetWorldState();
    void StartNewGame(const std::string &saveName);
    void LoadExistingGame(const std::string &saveName);
    bool LoadSave(const std::string &saveName);
    void SaveCurrentGame();
    void BeginChunkLoad();
    void UpdateChunkStreaming(const glm::vec3 &center);
    void RefreshSaveList();

    // --- Screens ---
    void RunMainMenu();
    void RunPauseMenu();
    void RunLoadingScreen();

    // Core systems
    std::unique_ptr<AudioEngine> m_AudioEngine;
    std::unique_ptr<AudioEventSystem> m_AudioEvents;
    std::unique_ptr<VulkanEngine> m_VulkanEngine;
    std::unique_ptr<RenderSystem> m_RenderSystem;
    std::unique_ptr<Camera3D> m_Camera;
    std::unique_ptr<entt::registry> m_Registry;
    std::unique_ptr<PlacementSystem> m_PlacementSystem;
    std::shared_ptr<SkinnedMesh> m_UnitMesh;
    GameSettings m_Settings;
    InputState m_Input;
    GameHUD m_HUD;

    // State machine
    GameState m_State = GameState::MainMenu;
    bool m_IsRunning = false;
    bool m_Initialized = false;
    bool m_PendingLoad = false;   // reload the current save at the start of the next Playing frame
    bool m_PendingMenu = false;   // return to the main menu at the start of the next Playing frame
    bool m_GameOver = false;

    // Saves / menus
    std::string m_CurrentSaveName;
    std::vector<std::string> m_AvailableSaves;
    char m_NewGameNameBuffer[64] = "MyWorld";
    bool m_ShowOptionsInPause = false;
    bool m_ShowOptionsInMenu = false;

    // World
    WorldState m_World;
    ChunkManager m_ChunkManager;
    DayNightCycle m_DayNightCycle;
    NestManager m_Nests;
    bool m_HadCatalyst = false;   // set by VictorySystem once a Catalyst has existed; losing it = game over
    float m_FogTimer = 0.0f;
    float m_TotalTime = 0.0f;

    // Player / hotbar
    entt::entity m_PlayerEntity = entt::null;
    entt::entity m_InspectedEntity = entt::null;
    static constexpr int HOTBAR_SIZE = 9;
    ItemId m_Hotbar[HOTBAR_SIZE] = { ItemId::Wood, ItemId::CopperOre, ItemId::IronOre, ItemId::Miner, ItemId::Furnace,
                                     ItemId::Assembler, ItemId::Belt, ItemId::Inserter, ItemId::None };
    ItemId m_SelectedItem = ItemId::None;

    // Free-fly mouse look
    bool m_FirstMouse = true;
    double m_LastMouseX = 0.0, m_LastMouseY = 0.0;

    // RTS selection
    std::vector<entt::entity> m_SelectedUnits;
    bool m_IsBoxSelecting = false;
    glm::vec2 m_BoxSelectStart{};   // physical pixels, used for the selection math
    glm::vec2 m_BoxSelectStartUI{}; // ImGui space, used only for drawing the rectangle

    // Debug
    int m_DebugDamageType = 0;
};
