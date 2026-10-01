#pragma once
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include "../Utils/GameSettings.h"
#include "../Audio/AudioEngine.h"
#include "../Audio/AudioEventSystem.h"
#include "ItemDatabase.h"
#include "../World/ResourceMap.h"
#include "../World/WorldGenerator.h"
#include "../Systems/MinerSystem.h"
#include "../World/PlacementGrid.h"
#include "TechState.h"
#include "../World/DayNightCycle.h"
#include "ChunkManager.h"
#include "../World/DepletionMap.h"
#include "../World/RemovedTreesMap.h"
#include "../Renderer/TerrainRenderer.h"
#include "../World/TreeHealthMap.h"
class TerrainGenerator;
class VulkanEngine;
class RenderSystem;
class Camera3D;
class AudioEngine;
class MinerSystem;
class PlaceableDatabase;
class PlacementSystem;


struct GLFWwindow;

enum class GameState {
    MainMenu,
    Playing,
    Paused,
    GameOver,
    Loading,
};

class Game {
public:
    Game();
    ~Game();
    void Run();

private:
    void Init();
    void LoadResources();
    void CreateInitialEntities();
    void HandleInput(float deltaTime);
    void HandleIsoInput(GLFWwindow* window, float deltaTime);
    void HandleFreeFlyInput(GLFWwindow* window, float deltaTime);
    void MovePlayer(glm::vec3 direction, float deltaTime);
    void Update(float deltaTime);
    void Render();
    void Shutdown();
    // Core systems
    std::unique_ptr<AudioEngine> m_AudioEngine;
    std::unique_ptr<AudioEventSystem> m_AudioEvents;
    std::unique_ptr<VulkanEngine> m_VulkanEngine;
    std::unique_ptr<RenderSystem> m_RenderSystem;
    std::unique_ptr<Camera3D> m_Camera;
    std::unique_ptr<entt::registry> m_Registry;
    std::unique_ptr<PlacementSystem> m_PlacementSystem;
    ItemId m_SelectedItem = ItemId::None;   // whatever "hotbar slot" logic you build later selects this
    static constexpr int HOTBAR_SIZE = 9;
    ItemId m_Hotbar[HOTBAR_SIZE] = { ItemId::Wood, ItemId::CopperOre, ItemId::IronOre, ItemId::Miner, ItemId::Furnace, ItemId::Assembler, ItemId::Belt, ItemId::Inserter, ItemId::None };
    int m_SelectedHotbarSlot = -1;   // -1 = nothing selected
    ResourceMap m_ResourceMap;
    PlacementGrid m_PlacementGrid;
    TechState m_TechState;

    ItemId GetSelectedItem() const {
        return (m_SelectedHotbarSlot >= 0 && m_SelectedHotbarSlot < HOTBAR_SIZE) ? m_Hotbar[m_SelectedHotbarSlot] : ItemId::None;
    }
    GameState m_State = GameState::MainMenu;
    std::string m_CurrentSaveName; // which save file is active, once in Playing state
    RemovedTreesMap m_RemovedTreesMap;
    void RunMainMenu();
    void StartNewGame(const std::string &saveName);
    void LoadExistingGame(const std::string &saveName);
    void RunPauseMenu();
    void RunLoadingScreen();
    void QuitToMenu(); // for later, if you add a "return to menu" option mid-game
    bool m_ShowOptionsInPause = false;
    bool m_ShowOptionsInMenu = false;
    std::vector<std::string> m_AvailableSaves;
    char m_NewGameNameBuffer[64] = "MyWorld";
    glm::vec3 m_LastPlayerPosition = glm::vec3(0.0f);
    void RefreshSaveList();
    ChunkManager m_ChunkManager;
    entt::entity m_PlayerEntity = entt::null;
    entt::entity m_TerrainEntity = entt::null;
    entt::entity m_CurrentTarget = entt::null;
    GameSettings m_Settings;
    bool m_FirstMouse = true;
    double m_LastMouseX = 0.0, m_LastMouseY = 0.0;
    DayNightCycle m_DayNightCycle;
    DepletionMap m_DepletionMap;
    bool m_IsRunning = false;
    bool m_Initialized = false;
    entt::entity m_InspectedEntity = entt::null;
    float m_LoadingProgress = 0.0f; // 0.0 to 1.0
    TreeHealthMap m_TreeHealthMap;
    std::unique_ptr<TerrainRenderer> m_TerrainRenderer;
    bool m_TerrainTestActive = false;


    struct NoiseTestPush {
        float startX;
        float startZ;
        float spacing;
        uint32_t sampleType;   // 0 = raw simplex, 1 = fbm, 2 = ridged
    };
    void RunNoiseLibraryTest();

    struct BiomeTestPush {
        float startX;
        float startZ;
        float spacing;
        float seed;
        uint32_t sampleType;
        float worldExtentZ;
    };

    void RunBiomeLibraryTest();
    void RunSimplexDebugTest(glm::vec2 testPoint);
    void RunRidgedDebugTest(glm::vec2 pos, int octaves, float frequency, float lacunarity, float gain);
    void RunBiomeGridTest();
};