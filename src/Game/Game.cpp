#include "Game.h"
#include <filesystem>
#include "../Engine/VulkanEngine.h"
#include "../Renderer/RenderSystem.h"
#include "../Renderer/DebugLineRenderer.h"
#include <iostream>
#include <glm/glm.hpp>
#include "../Components/Components.h"
#include "Camera3D.h"
#include "../Rendering/ModelLoader.h"
#include "../World/TerrainGenerator.h"
#include "../Helpers/DebugUI.h"
#include "../Utils/GameSettings.h"
#include <imgui/imgui.h>
#include "../Systems/InteractionSystem.h"
#include "ItemDatabase.h"
#include "PlaceableDatabase.h"
#include "../Systems/PlacementSystem.h"
#include "RecipeDatabase.h"
#include "../World/TerrainRaycast.h"
#include "../Systems/MinerSystem.h"
#include "FurnaceRecipeDatabase.h"
#include "../Systems/FurnaceSystem.h"
#include "../Systems/AssemblerSystem.h"
#include "../Systems/BeltSystem.h"
#include "../Systems/InserterSystem.h"
#include "../Systems/MoveOrderSystem.h"
#include "FuelDatabase.h"
#include "../Systems/PowerSystem.h"
#include "../Helpers/SaveManager.h"
#include "../Renderer/ImGuiVulkanUtil.h"
#include "../Helpers/PauseMenuAction.h"
#include "../Rendering/SkinnedMesh.h"
#include "../Systems/AnimationSystem.h"
#include "../Systems/PlayerAnimationSystem.h"
#include "BiomeDatabase.h"
#include "OreDepositMap.h"
#include "../Rendering/TerrainBufferPool.h"
#include "../Rendering/ChunkCuller.h"
#include "../Systems/BeltRenderSystem.h"
#include "../Renderer/TerrainRenderer.h"
#include "../Rendering/ChunkGpuMetadata.h"
#include "ChunkManager.h"
#include "../World/TreePlacement.h"
#include "../Utils/GpuNoiseMatch.h"
#include "FogOfWar.h"
#include "../Rendering/FogTexture.h"
#include <chrono>
#include "../Renderer/TreeRenderer.h" 
#include "../Systems/DamageSystem.h"
#include "ResistancePresets.h"

Game::Game() {

}
Game::~Game() {
    Shutdown();
}

void Game::Run() {
    Init();
    m_IsRunning = true;
    float lastTime = static_cast<float>(glfwGetTime());

    while (m_IsRunning && !m_VulkanEngine->ShouldClose()) {
        m_VulkanEngine->PollEvents();

        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        GLFWwindow *window = m_VulkanEngine->GetWindow();

        // Esc toggle — works regardless of Playing/Paused, checked once here rather than duplicated in both states
        static bool escWasDown = false;
        bool escIsDown = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
        if (escIsDown && !escWasDown) {
            if (m_State == GameState::Playing)
                m_State = GameState::Paused;
            else if (m_State == GameState::Paused)
                m_State = GameState::Playing;
        }
        escWasDown = escIsDown;

        if (m_State == GameState::MainMenu) {
            RunMainMenu();
        } else if (m_State == GameState::Playing) {
            HandleInput(deltaTime);
            Update(deltaTime);
            Render();
        } else if (m_State == GameState::Paused) {
            RunPauseMenu(); // renders the frozen last frame + pause UI on top, no gameplay update
        } else if (m_State == GameState::Loading) {
        m_ChunkManager.ProcessCompletedChunks(*m_Registry, m_VulkanEngine.get(), m_RenderSystem->GetMeshRenderer(),
                                              m_PlacementGrid, m_RemovedTreesMap);

        RunLoadingScreen(); // renders progress bar

        if (m_ChunkManager.IsInitialLoadComplete()) {
            m_State = GameState::Playing;
        }
    }
    }

    Shutdown();
}

void Game::Init() {



    std::cout << "=== Loading Settings ===" << std::endl;
    m_Settings = GameSettings::LoadFromFile("settings.json");
    std::cout << "=== Settings Loaded ===" << std::endl;

    std::cout << "=== Registering item Database ===" << std::endl;
    ItemDatabase::Init();
    RecipeDatabase::Init();
    FuelDatabase::Init();

    PlaceableDatabase::Init();
    m_PlacementSystem = std::make_unique<PlacementSystem>();

    FurnaceRecipeDatabase::Init();
    //OreDatabase::Init();
    BiomeDatabase::Init();

    m_ChunkManager.StartWorkerThread(m_Settings.terrain, m_Settings.terrain.seed);

    std::cout << "=== Initializing Engine ===" << std::endl;
    m_VulkanEngine = std::make_unique<VulkanEngine>();
    m_VulkanEngine->Init("Game", 1280, 720);

    std::cout << "=== Initializing Audio Engine ===" << std::endl;
    m_AudioEngine = std::make_unique<AudioEngine>();
    m_AudioEngine->Init();

    m_AudioEvents = std::make_unique<AudioEventSystem>(m_AudioEngine.get());
    m_AudioEvents->RegisterSound(AudioEvent::TreeChopped, "Assets/Audio/Hi Seed Shaker 1.wav");
    m_AudioEvents->RegisterSound(AudioEvent::OreCollected, "Assets/Audio/Lo Seed Shaker 1.wav");
    m_AudioEvents->RegisterSound(AudioEvent::UIClick, "Assets/Audio/Tambourine 3.wav");

    m_AudioEngine->PlayMusic("Assets/Audio/GremlinRapFin.mp3", true, m_Settings.masterVolume);

    m_AudioEngine->SetMasterVolume(m_Settings.masterVolume);
    m_AudioEngine->SetMusicVolume(m_Settings.musicVolume);
    m_AudioEngine->SetSFXVolume(m_Settings.sfxVolume);
    std::cout << "=== Initializing Render Engine ===" << std::endl;


    m_RenderSystem = std::make_unique<RenderSystem>(m_VulkanEngine.get());
    uint32_t vertsPerChunk = ChunkManager::CHUNK_VERTEX_RESOLUTION * ChunkManager::CHUNK_VERTEX_RESOLUTION;
    uint32_t indicesPerChunk = (ChunkManager::CHUNK_VERTEX_RESOLUTION - 1) * (ChunkManager::CHUNK_VERTEX_RESOLUTION - 1) * 6;

    const float maxZoomFactor = ChunkManager::MAX_ZOOM_FACTOR;   // MUST match ChunkManager::Update's maxZoomFactor exactly
    int scaledRadius = static_cast<int>(ChunkManager::LOAD_RADIUS_CHUNKS * maxZoomFactor);
    uint32_t maxChunks = (scaledRadius * 2 + 1) * (scaledRadius * 2 + 1) + 200;   // full worst-case square + buffer

 
    m_RenderSystem->Init(vertsPerChunk, indicesPerChunk, maxChunks);
    m_Camera = std::make_unique<Camera3D>(glm::vec3(0.0f, 5.0f, 5.0f));

    //m_Minimap.Init(m_VulkanEngine.get());
    m_VulkanEngine->ChainScrollCallback(); 
    m_Minimap.Init(m_VulkanEngine.get(), m_RenderSystem->GetFogTexture());
    m_RenderSystem->SetPreRenderCallback([this](VkCommandBuffer cmd) {
        m_Minimap.Record(cmd, GetMinimapCenter(), m_MinimapRange, (float)m_Settings.terrain.seed);
    });
    m_RenderSystem->SetUICallback([this]() { DrawMinimap(); }); // ADD

    //glfwSetInputMode(m_VulkanEngine->GetWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);



    //Test

    //RunNoiseLibraryTest();
   // float worldExtentZ = m_Settings.terrain.gridDepth * m_Settings.terrain.cellSize;   // = 100
   // std::cout << "worldExtentZ = " << worldExtentZ << std::endl;
    //std::cout << "m_Settings.terrain.seed = " << m_Settings.terrain.seed << std::endl;
    //RunBiomeLibraryTest();
    //float seedOffset = GpuNoiseMatch::HashSeed((float)m_Settings.terrain.seed);
    //glm::vec2 testInput = glm::vec2(-32.0f, 312.0f) * 0.0008f + glm::vec2(seedOffset, seedOffset);
    //std::cout << "m_Settings.terrain.seed = " << m_Settings.terrain.seed << std::endl;
    //RunSimplexDebugTest(testInput);
    //GpuNoiseMatch::SimplexNoise2D(testInput);
    //
    //std::cout << "m_Settings.terrain.seed = " << m_Settings.terrain.seed << std::endl;
    ////float seedOffset = GpuNoiseMatch::HashSeed((float)m_Settings.terrain.seed);
    //std::cout << "New HashSeed(2143) = " << seedOffset << std::endl;   // sanity check — should roughly match whatever biome_test.comp now reports internally
    //glm::vec2 testPos = glm::vec2(-32.0f, 312.0f) * 0.0008f + glm::vec2(seedOffset, seedOffset);    RunRidgedDebugTest(testPos, 3, 1.0f, 2.0f, 0.5f);
    //float cpuResult = GpuNoiseMatch::RidgedNoise(testPos, 3, 1.0f, 2.0f, 0.5f);   // will print each octave via the added std::cout lines
    //std::cout << "CPU final result=" << cpuResult << std::endl;
    //float cpuTrigger = GpuNoiseMatch::GetElevationTrigger(-32.0f, 312.0f, 2143.0f);
    //std::cout << "CPU full GetElevationTrigger = " << cpuTrigger << std::endl;
    //COmment to build
   // RunBiomeGridTest();

    m_Registry = std::make_unique<entt::registry>();

 
    //CreateInitialEntities();
    m_Initialized = true;
    std::cout << "=== Engine Initialized ===" << std::endl;
}


void Game::LoadResources() {
    // Load game resources (textures, meshes, etc.) here
    std::cout << "Loading resources..." << std::endl;

    std::cout << "Resources loaded" << std::endl;
}
void Game::CreateInitialEntities() {
    // Create initial game entities and components here
    std::cout << "Creating initial entities..." << std::endl;

        std::cout << "Terrain settings: gridWidth=" << m_Settings.terrain.gridWidth
              << " gridDepth=" << m_Settings.terrain.gridDepth
              << " cellSize=" << m_Settings.terrain.cellSize
              << " heightScale=" << m_Settings.terrain.heightScale
              << " noiseScale=" << m_Settings.terrain.noiseScale
              << " seed=" << m_Settings.terrain.seed << std::endl;


        auto entity = m_Registry->create();

        auto &sprite = m_Registry->emplace<SpriteComponent>(entity);
        sprite.transform.Position = glm::vec3(150.0f, 150.0f, 0.0f);
        sprite.transform.Scale = glm::vec3(300.0f, 600.0f, 1.0f);
        sprite.color = glm::vec4(0.3f, 0.2f, 0.1f, 0.9f);
        sprite.layer = 0;
        m_Registry->emplace<NameTag>(entity, "UI Background");
    
        std::mt19937 rng(m_Settings.terrain.seed);
        glm::vec3 spawnPos = WorldGenerator::FindSpawnPoint(m_Settings.terrain, m_Settings.terrain.seed, rng);


        m_PlayerEntity = m_Registry->create();
        auto &playerTransform = m_Registry->emplace<TransformComponent>(m_PlayerEntity);
        playerTransform.Position = spawnPos;
        m_Registry->emplace<PlayerComponent>(m_PlayerEntity);
        m_Registry->emplace<InventoryComponent>(m_PlayerEntity);
        m_Registry->emplace<SelectableComponent>(m_PlayerEntity);

        m_Registry->emplace<BoundsComponent>(m_PlayerEntity, BoundsComponent{ glm::vec3(0.5f, 1.0f, 0.5f) });
        auto playerMesh = std::make_shared<SkinnedMesh>(ModelLoader::LoadSkinnedModel("Assets/Models/Test1.glb", m_VulkanEngine.get(), m_RenderSystem->GetMeshRenderer())); // swap for a real player model later
        m_Registry->emplace<AnimationComponent>(m_PlayerEntity);
        m_Registry->emplace<SkinnedMeshComponent>(m_PlayerEntity, playerMesh);
        m_Registry->emplace<NameTag>(m_PlayerEntity, "Player");
        m_Registry->emplace<VisionComponent>(m_PlayerEntity, 256.0f);
        m_Registry->emplace<TeamComponent>(m_PlayerEntity,0);
        m_Registry->emplace<PlayerTag>(m_PlayerEntity);
        m_Registry->emplace<HealthComponent>(m_PlayerEntity);
        m_Registry->emplace<ResistanceComponent>(m_PlayerEntity, ResistancePresets::Player());

        m_ResourceMap.Generate(m_Settings.terrain.gridWidth, m_Settings.terrain.gridDepth,
            m_Settings.terrain.cellSize, m_Settings.terrain.seed);



        
       /* auto terrainMesh = TerrainGenerator::GenerateHeightmapTerrain(
            m_Settings.terrain, m_ResourceMap);
        std::cout << "Terrain mesh vertex count: " << terrainMesh->Vertices.size()
                  << " index count: " << terrainMesh->Indices.size() << std::endl;
        m_TerrainEntity = m_Registry->create();
        m_Registry->emplace<TransformComponent>(m_TerrainEntity);
        m_Registry->emplace<MeshComponent>(m_TerrainEntity, terrainMesh);
        m_Registry->emplace<NameTag>(m_TerrainEntity, "Terrain");

        WorldGenerator::ScatterTrees(*m_Registry, m_Settings.terrain, m_ResourceMap, m_VulkanEngine.get(), m_RenderSystem->GetMeshRenderer());
        */
        }
void Game::HandleInput(float deltaTime) {
    GLFWwindow* window = m_VulkanEngine->GetWindow();

    // --- Mode toggle (F2), edge-detected like your F1 UI toggle ---
    static bool f1WasDown = false, f2WasDown = false;
    bool f2IsDown = glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS;
    if (f2IsDown && !f2WasDown) {
        bool nowIso = m_Camera->GetMode() == Camera3D::Mode::Isometric;
        m_Camera->SetMode(nowIso ? Camera3D::Mode::FreeFly : Camera3D::Mode::Isometric);

        // Reset mouse delta tracking so switching modes doesn't cause a sudden jump
        // if the cursor moved while the other mode was inactive.
        m_FirstMouse = true;
    }
    f2WasDown = f2IsDown;
 
    if (m_Camera->GetMode() == Camera3D::Mode::Isometric || m_Camera->GetMode() == Camera3D::Mode::IsoRTS) {
        HandleIsoInput(window, deltaTime);
    }
    else {
        HandleFreeFlyInput(window, deltaTime);
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        m_IsRunning = false;
    }
    bool f1IsDown = glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS;
    if (f1IsDown && !f1WasDown) {
        m_AudioEvents->Trigger(AudioEvent::TreeChopped);
    }
    f1WasDown = f1IsDown;

    static bool escWasDown = false;
    bool escIsDown = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    if (escIsDown && !escWasDown) {
        if (m_State == GameState::Playing) {
            m_State = GameState::Paused;
        } else if (m_State == GameState::Paused) {
            m_State = GameState::Playing;
        }
    }
    escWasDown = escIsDown;

}
void Game::IssueMoveCommand(const glm::vec3& ground) {
    int n = static_cast<int>(m_SelectedUnits.size());
    int side = static_cast<int>(std::ceil(std::sqrt((float)std::max(n, 1))));
    int i = 0;
    for (auto unit : m_SelectedUnits) {
        if (!m_Registry->valid(unit)) continue;
        glm::vec3 offset((i % side - (side - 1) * 0.5f) * 2.0f, 0.0f,
            (i / side - (side - 1) * 0.5f) * 2.0f);
        m_Registry->emplace_or_replace<MoveOrderComponent>(unit, MoveOrderComponent{ ground + offset });
        i++;
    }
}

void Game::HandleIsoInput(GLFWwindow* window, float deltaTime) {
    static bool qWasDown = false, eWasDown = false, f11WasDown = false, interactWasDown = false, 
        placeWasDown = false, rotateWasDown = false, inspectWasDown = false, rotatePlacedWasDown = false, pickupWasDown = false, saveWasDown = false,
        loadWasDown = false, tabWasDown = false, kWasDown = false;
    
     double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);   // logical size
    VkExtent2D extent = m_VulkanEngine->GetSwapChainExtent();   // physical size (framebuffer)

    float dpiScaleX = (windowWidth > 0) ? static_cast<float>(extent.width) / windowWidth : 1.0f;
    float dpiScaleY = (windowHeight > 0) ? static_cast<float>(extent.height) / windowHeight : 1.0f;

    mx *= dpiScaleX;   // NEW — convert logical cursor position to physical pixel position
    my *= dpiScaleY;   // NEW

    float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);


    glm::vec3 rayOrigin = m_Camera->GetEyePosition();
    glm::vec3 rayDir = m_Camera->ScreenPointToRay(static_cast<float>(mx), static_cast<float>(my),
                                                  static_cast<float>(extent.width), static_cast<float>(extent.height), aspect);

    auto &playerTransform = m_Registry->get<TransformComponent>(m_PlayerEntity);
    float interactRange = 25.0f;

    entt::entity machineTarget = InteractionSystem::FindMachineAlongRay(*m_Registry, rayOrigin, rayDir, 100.0f);

    bool pickupIsDown = glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS; // pick a free key
    bool rotatePlacedIsDown = glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS; // separate key from ghost-rotation R
    bool qIsDown = glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS;
    bool eIsDown = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
    bool f11IsDown = glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS;
    bool interactIsDown = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
    bool placeIsDown = glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS;
    bool rotateIsDown = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
    bool inspectIsDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    bool saveIsDown = glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS;
    bool loadIsDown = glfwGetKey(window, GLFW_KEY_F9) == GLFW_PRESS;
    bool tabIsDown = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;

    //Testing
    static bool f10WasDown = false;
    bool f10Down = glfwGetKey(window, GLFW_KEY_F10) == GLFW_PRESS;
    if (f10Down && !f10WasDown && m_Registry->valid(m_PlayerEntity)) {
        auto& pt = m_Registry->get<TransformComponent>(m_PlayerEntity);
        CompareTerrainHeightAt(pt.Position.x, pt.Position.z);
        std::cout << "player Position.y=" << pt.Position.y << std::endl;
    }
    f10WasDown = f10Down;

    static int typeIdx = 0;
    bool kDown = glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS;
    if (kDown && !kWasDown && m_Registry->valid(m_PlayerEntity)) {
        DamageType t = static_cast<DamageType>(typeIdx % DAMAGE_TYPE_COUNT);
        float before = m_Registry->get<HealthComponent>(m_PlayerEntity).current;
        DamageSystem::ApplyDamage(*m_Registry, m_PlayerEntity, 20.0f, t);
        std::cout << "type " << typeIdx % DAMAGE_TYPE_COUNT << " dealt "
                  << before - m_Registry->get<HealthComponent>(m_PlayerEntity).current << std::endl;
        typeIdx++;
    }

    //Testing




    if (placeIsDown && !placeWasDown) m_PlacementSystem->TryConfirmPlacement(*m_Registry, m_PlayerEntity, m_PlacementGrid, m_Settings.terrain);
    if (qIsDown && !qWasDown) m_Camera->SnapRotateIso(false);
    if (eIsDown && !eWasDown) m_Camera->SnapRotateIso(true);
    if (f11IsDown && !f11WasDown) m_VulkanEngine->ToggleFullscreen();
    if (rotateIsDown && !rotateWasDown) {
        m_PlacementSystem->RotateGhost();
    }
    if (interactIsDown && !interactWasDown) {
       
 

        if (m_Registry->valid(machineTarget)) {
            auto &machineTransform = m_Registry->get<TransformComponent>(machineTarget);
            float dist = glm::length(machineTransform.Position - playerTransform.Position);

            if (dist <= interactRange) {
                // Try collecting first
                bool collected = false;
                if (m_Registry->any_of<MinerComponent>(machineTarget)) {
                    collected = InteractionSystem::TryCollectMinerOutput(*m_Registry, machineTarget, m_PlayerEntity);
                } else {
                    collected = InteractionSystem::TryCollectFromMachine(*m_Registry, machineTarget, m_PlayerEntity);
                }
                if (!collected) {
                    ItemId selected = m_SelectedItem;
                    if (selected != ItemId::None) {
                        bool fueledSomething = false;
                        if (m_Registry->any_of<FurnaceComponent>(machineTarget)) {
                            fueledSomething = InteractionSystem::TryFuelFurnace(*m_Registry, machineTarget, m_PlayerEntity, selected, 1);
                        } else if (m_Registry->any_of<PowerGeneratorComponent>(machineTarget)) {
                            fueledSomething = InteractionSystem::TryFuelGenerator(*m_Registry, machineTarget, m_PlayerEntity, selected, 1);
                        } else if (m_Registry->any_of<MinerComponent>(machineTarget)) {
                            fueledSomething = InteractionSystem::TryFuelMiner(*m_Registry, machineTarget, m_PlayerEntity, selected, 1);
                        }
                       
                        if (!fueledSomething) {
                            InteractionSystem::TryInsertIntoMachine(*m_Registry, machineTarget, m_PlayerEntity, selected, 1);
                        }
                    }
                }
            }
        } else {
            InteractionSystem::TryMineAtCursor(*m_Registry, m_DepletionMap, m_PlayerEntity,
                                               rayOrigin, rayDir, m_Settings.terrain, interactRange, m_AudioEvents.get(),
                                               m_VulkanEngine.get(), m_RenderSystem->GetMeshRenderer(), m_PlacementGrid, m_RemovedTreesMap, m_ChunkManager, m_TreeHealthMap, *m_RenderSystem->GetTreeRenderer());
        }
    }
    


    if (inspectIsDown && !inspectWasDown && !ImGui::GetIO().WantCaptureMouse) {
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);   // logical size
        VkExtent2D extent = m_VulkanEngine->GetSwapChainExtent();   // physical size (framebuffer)

        float dpiScaleX = (windowWidth > 0) ? static_cast<float>(extent.width) / windowWidth : 1.0f;
        float dpiScaleY = (windowHeight > 0) ? static_cast<float>(extent.height) / windowHeight : 1.0f;

        mx *= dpiScaleX;   // NEW — convert logical cursor position to physical pixel position
        my *= dpiScaleY;   // NEW

        float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);

        glm::vec3 rayOrigin = m_Camera->GetEyePosition();
        glm::vec3 rayDir = m_Camera->ScreenPointToRay((float)mx, (float)my, (float)extent.width, (float)extent.height, aspect);

        m_InspectedEntity = InteractionSystem::FindMachineAlongRay(*m_Registry, rayOrigin, rayDir, 100.0f);
    }




    if (rotatePlacedIsDown && !rotatePlacedWasDown) {

        if (m_Registry->valid(machineTarget)) {
            InteractionSystem::TryRotateMachine(*m_Registry, machineTarget);
        }
    }
    if (pickupIsDown && !pickupWasDown) {
        if (m_Registry->valid(machineTarget)) { // reuse the same machineTarget you compute for interact
            InteractionSystem::TryPickupMachine(*m_Registry, machineTarget, m_PlayerEntity, m_PlacementGrid, m_Settings.terrain.cellSize );
        }
    }

  
    if (saveIsDown && !saveWasDown) {
        SaveManager::SaveGame("Saves/" + m_CurrentSaveName + ".json", *m_Registry, m_PlayerEntity, m_ResourceMap, m_TechState, m_Settings, m_VulkanEngine.get(), m_RenderSystem->GetMeshRenderer(), m_DepletionMap, m_RemovedTreesMap);
    }
    saveWasDown = saveIsDown;
    if (loadIsDown && !loadWasDown) {
        m_Registry->clear();
        m_PlacementGrid = PlacementGrid{};
        m_DepletionMap = DepletionMap{};
        m_RemovedTreesMap = RemovedTreesMap{};

        entt::entity loadedPlayer;
        if (SaveManager::LoadGame("Saves/" + m_CurrentSaveName + ".json", *m_Registry, loadedPlayer, m_ResourceMap, m_PlacementGrid, m_TechState, m_Settings, m_VulkanEngine.get(), m_RenderSystem->GetMeshRenderer(), m_DepletionMap, m_RemovedTreesMap)) {
            m_PlayerEntity = loadedPlayer;
        }
    }
    if (tabIsDown && !tabWasDown) {
        if (m_Camera->GetMode() == Camera3D::Mode::IsoRTS) {
            m_Camera->SetMode(Camera3D::Mode::Isometric);   // isoTarget stays exactly where it currently is — seamless transition

            auto selectables = m_Registry->view<SelectableComponent>();
            for (auto e : selectables) selectables.get<SelectableComponent>(e).isSelected = false;
            m_SelectedUnits.clear();
            m_IsBoxSelecting = false;
            m_LeftWasDown = false;
            m_RightWasDown = false;
            m_RenderSystem->SetSelectionRect(false, {}, {});
        }
        else {
            m_Camera->SetMode(Camera3D::Mode::IsoRTS);
        }
    }

    qWasDown = qIsDown;
    eWasDown = eIsDown;
    f11WasDown = f11IsDown;
    interactWasDown = interactIsDown;
    placeWasDown = placeIsDown;
    rotateWasDown = rotateIsDown;
    inspectWasDown = inspectIsDown;
    rotatePlacedWasDown = rotatePlacedIsDown;
    pickupWasDown = pickupIsDown;
    saveWasDown = saveIsDown;
    loadWasDown = loadIsDown;
    tabWasDown = tabIsDown;

    if (m_Camera->GetMode() == Camera3D::Mode::IsoRTS) {
        HandleRTSInput(window, deltaTime);
        glm::vec3 panDir(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) panDir.z -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) panDir.z += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) panDir.x += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) panDir.x -= 1.0f;
        bool runHeld = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
        if (glm::length(panDir) > 0.001f) {
            m_Camera->PanIso(glm::normalize(panDir), deltaTime, runHeld);
        }
        
    }
    else if (m_Camera->GetMode() == Camera3D::Mode::Isometric)
    {
        glm::vec3 moveDir(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveDir.z += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveDir.z -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveDir.x -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveDir.x += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) m_Camera->ProcessKeyboard(CameraMovement::Up, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) m_Camera->ProcessKeyboard(CameraMovement::Down, deltaTime);
        // Game::HandleIsoInput
        if (glm::length(moveDir) > 0.0f && m_Registry->valid(m_PlayerEntity)) {
            m_Registry->remove<MoveOrderComponent>(m_PlayerEntity);   // manual input overrides any queued order
            MovePlayer(glm::normalize(moveDir), deltaTime);
        }
    }
    float scrollDelta = m_VulkanEngine->GetScrollDelta();
    if (scrollDelta != 0.0f && !ImGui::GetIO().WantCaptureMouse) {
        //std::cout << "process iso zoom " << scrollDelta * m_Settings.isoZoomSpeed << std::endl;
        m_Camera->ProcessIsoZoom(scrollDelta * m_Settings.isoZoomSpeed);
    }
    m_VulkanEngine->ResetScrollDelta();
    
    for (int i = 0; i < HOTBAR_SIZE; i++) {
        static bool numWasDown[HOTBAR_SIZE] = { false };
        bool numIsDown = glfwGetKey(window, GLFW_KEY_1 + i) == GLFW_PRESS;
        if (numIsDown && !numWasDown[i]) {
            ItemId hotbarItem = m_Hotbar[i];
            m_SelectedItem = (m_SelectedItem == hotbarItem) ? ItemId::None : hotbarItem;
        }
        numWasDown[i] = numIsDown;
    }


    

    m_Camera->UpdateIso(deltaTime);
}

void Game::MovePlayer(glm::vec3 direction, float deltaTime) {
    auto& transform = m_Registry->get<TransformComponent>(m_PlayerEntity);
    auto& player = m_Registry->get<PlayerComponent>(m_PlayerEntity);
    player.moveSpeed = m_Settings.playerMoveSpeed; // copied ONCE at creation
    player.runSpeed = m_Settings.playerRunSpeed;
    // Move relative to the camera's current facing, same axis logic as the old iso pan —
    // so "forward" is always "up the screen" regardless of which 45° snap we're on.
    float camYaw = m_Camera->GetIsoYaw();   // needs a small getter — see below
    glm::vec3 forward = glm::normalize(glm::vec3(-cos(glm::radians(camYaw)), 0.0f, -sin(glm::radians(camYaw))));
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
	if (glfwGetKey(m_VulkanEngine->GetWindow(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
		transform.Position += (forward * direction.z + right * direction.x) * player.runSpeed * deltaTime;
	}
	else {
		transform.Position += (forward * direction.z + right * direction.x) * player.moveSpeed * deltaTime;
	}

    float cellSize = ChunkManager::CHUNK_WORLD_SIZE / (ChunkManager::CHUNK_VERTEX_RESOLUTION - 1);
   // std::cout << "exact=" << GpuNoiseMatch::SampleHeight(transform.Position.x, transform.Position.z, (float)m_Settings.terrain.seed)    
   //     << " mesh=" << GpuNoiseMatch::SampleMeshHeight(transform.Position.x, transform.Position.z, (float)m_Settings.terrain.seed, cellSize);
    transform.Position.y = GpuNoiseMatch::SampleMeshHeight(transform.Position.x, transform.Position.z, (float)m_Settings.terrain.seed, cellSize);
}

void Game::HandleRTSInput(GLFWwindow* window, float deltaTime) {    
    ImGuiIO& io = ImGui::GetIO();

    // Cursor in physical pixels (same DPI correction as HandleIsoInput)
    double rawX, rawY;
    glfwGetCursorPos(window, &rawX, &rawY);
    int winW, winH;
    glfwGetWindowSize(window, &winW, &winH);
    VkExtent2D extent = m_VulkanEngine->GetSwapChainExtent();
    float mx = static_cast<float>(rawX) * (winW > 0 ? (float)extent.width / winW : 1.0f);
    float my = static_cast<float>(rawY) * (winH > 0 ? (float)extent.height / winH : 1.0f);
    float aspect = static_cast<float>(extent.width) / extent.height;

    bool leftDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    bool rightDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

    // Don't let clicks on UI windows start or finish a selection
    bool uiCaptured = io.WantCaptureMouse && !m_IsBoxSelecting;

    // ---- Left mouse: press starts a drag, release finalizes ----
    if (leftDown && !m_LeftWasDown && !uiCaptured) {
        m_IsBoxSelecting = true;
        m_BoxSelectStart = glm::vec2(mx, my);
        m_BoxSelectStartUI = glm::vec2(io.MousePos.x, io.MousePos.y);   // was: = io.MousePos;    
    }

    if (!leftDown && m_LeftWasDown && m_IsBoxSelecting) {
        glm::vec2 boxEnd(mx, my);
        bool additive = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

        auto selectables = m_Registry->view<TransformComponent, SelectableComponent>();

        if (!additive) {
            for (auto e : selectables) selectables.get<SelectableComponent>(e).isSelected = false;
            m_SelectedUnits.clear();
        }

        auto addToSelection = [&](entt::entity e) {
            auto& sel = m_Registry->get<SelectableComponent>(e);
            if (sel.isSelected) return;
            sel.isSelected = true;
            m_SelectedUnits.push_back(e);
            };

        float dragDist = glm::length(boxEnd - m_BoxSelectStart);
        if (dragDist < 5.0f) {
            // Single click: ray-pick the nearest selectable
            glm::vec3 rayOrigin = m_Camera->GetEyePosition();
            glm::vec3 rayDir = m_Camera->ScreenPointToRay(mx, my, (float)extent.width, (float)extent.height, aspect);
            entt::entity clicked = InteractionSystem::FindSelectableAlongRay(*m_Registry, rayOrigin, rayDir, 500.0f);
            if (m_Registry->valid(clicked)) addToSelection(clicked);
        }
        else {
            // Box select: project each unit to screen space
            glm::vec2 boxMin = glm::min(m_BoxSelectStart, boxEnd);
            glm::vec2 boxMax = glm::max(m_BoxSelectStart, boxEnd);
            glm::mat4 viewProj = m_Camera->GetProjectionMatrix(aspect) * m_Camera->GetActiveViewMatrix();

            for (auto e : selectables) {
                glm::vec3 worldPos = selectables.get<TransformComponent>(e).Position + glm::vec3(0, 1.0f, 0);
                glm::vec4 clip = viewProj * glm::vec4(worldPos, 1.0f);
                if (clip.w <= 0.0f) continue;   // behind the camera

                glm::vec2 ndc = glm::vec2(clip.x, clip.y) / clip.w;
                // GetProjectionMatrix already flips Y for Vulkan, so ndc.y = -1 is the top of the screen
                glm::vec2 screen((ndc.x * 0.5f + 0.5f) * extent.width,
                    (ndc.y * 0.5f + 0.5f) * extent.height);

                if (screen.x >= boxMin.x && screen.x <= boxMax.x &&
                    screen.y >= boxMin.y && screen.y <= boxMax.y) {
                    addToSelection(e);
                }
            }
        }
        m_IsBoxSelecting = false;
    }

    // ---- Right mouse: move order for every selected unit ----
    if (rightDown && !m_RightWasDown && !io.WantCaptureMouse) {
        glm::vec3 rayOrigin = m_Camera->GetEyePosition();
        glm::vec3 rayDir = m_Camera->ScreenPointToRay(mx, my, (float)extent.width, (float)extent.height, aspect);
        glm::vec3 ground = TerrainRaycast::RaycastToTerrain(rayOrigin, rayDir, m_Settings.terrain);

        // Fan multiple units out so they don't stack on one point

        IssueMoveCommand(ground);
    }

    // ---- Drag rectangle overlay ----

        bool showRect = m_IsBoxSelecting && leftDown &&
            glm::length(glm::vec2(io.MousePos.x, io.MousePos.y) - m_BoxSelectStartUI) > 5.0f;
        m_RenderSystem->SetSelectionRect(showRect, m_BoxSelectStartUI, glm::vec2(io.MousePos.x, io.MousePos.y));

    m_LeftWasDown = leftDown;
    m_RightWasDown = rightDown;
}



void Game::HandleFreeFlyInput(GLFWwindow* window, float deltaTime) {

    bool sprint = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ||
        (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);


    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) m_Camera->ProcessKeyboard(CameraMovement::Forward, deltaTime, sprint);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) m_Camera->ProcessKeyboard(CameraMovement::Backward, deltaTime, sprint);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) m_Camera->ProcessKeyboard(CameraMovement::Left, deltaTime, sprint);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) m_Camera->ProcessKeyboard(CameraMovement::Right, deltaTime, sprint);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) m_Camera->ProcessKeyboard(CameraMovement::Up, deltaTime, sprint);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) m_Camera->ProcessKeyboard(CameraMovement::Down, deltaTime, sprint);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) m_Camera->ProcessKeyboard(CameraMovement::Shift, deltaTime, sprint);
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    if (m_FirstMouse) {
        m_LastMouseX = xpos;
        m_LastMouseY = ypos;
        m_FirstMouse = false;
    }
    float deltaX = static_cast<float>(xpos - m_LastMouseX);
    float deltaY = static_cast<float>(m_LastMouseY - ypos);
    m_LastMouseX = xpos;
    m_LastMouseY = ypos;

    if (!ImGui::GetIO().WantCaptureMouse) {
        m_Camera->ProcessMouseMovement(deltaX, deltaY);
    }
}
void Game::Update(float deltaTime) {
    GLFWwindow* window = m_VulkanEngine->GetWindow();

    m_TotalTime += deltaTime;
    m_RenderSystem->GetFogTexture()->SetTime(m_TotalTime);

    if (m_Registry->valid(m_PlayerEntity)) {
        auto& transform = m_Registry->get<TransformComponent>(m_PlayerEntity);
        if(m_Camera->GetMode() == Camera3D::Mode::Isometric)
        {
            m_Camera->SetIsoTarget(transform.Position);
        }
        entt::entity nearbyPickup = InteractionSystem::FindNearestPickup(*m_Registry, transform.Position, 8.0f); // small radius
        if (m_Registry->valid(nearbyPickup)) {
            std::cout << "Game::Update registry address: " << m_Registry.get() << std::endl;
            InteractionSystem::CollectPickup(*m_Registry, nearbyPickup, m_PlayerEntity, m_AudioEvents.get());
        }
    }

    if (m_Registry->valid(m_PlayerEntity)) {
        auto &transform = m_Registry->get<TransformComponent>(m_PlayerEntity);
        m_CurrentTarget = InteractionSystem::FindNearestInteractable(*m_Registry, transform.Position, 20.0f); // 3 unit range
    }

    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);   // logical size
    VkExtent2D extent = m_VulkanEngine->GetSwapChainExtent();   // physical size (framebuffer)

    float dpiScaleX = (windowWidth > 0) ? static_cast<float>(extent.width) / windowWidth : 1.0f;
    float dpiScaleY = (windowHeight > 0) ? static_cast<float>(extent.height) / windowHeight : 1.0f;

    mx *= dpiScaleX;   // NEW — convert logical cursor position to physical pixel position
    my *= dpiScaleY;   // NEW

    float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);


    //auto t0 = std::chrono::high_resolution_clock::now();

    m_PlacementSystem->Update(*m_Registry, m_PlayerEntity, *m_Camera, m_SelectedItem,
        m_Settings.terrain, static_cast<float>(mx), static_cast<float>(my),
        static_cast<float>(extent.width), static_cast<float>(extent.height), aspect, m_PlacementGrid, m_VulkanEngine.get(), m_RenderSystem->GetMeshRenderer());
   // auto t1 = std::chrono::high_resolution_clock::now();
    //static int n = 0;
    //if (++n % 60 == 0)
    //    std::cout << "placement update ms: "
    //              << std::chrono::duration<double, std::milli>(t1 - t0).count() << std::endl;
    m_DayNightCycle.Update(deltaTime);

    MinerSystem::Update(*m_Registry, m_DepletionMap, m_Settings.terrain, deltaTime, m_ChunkManager);
    FurnaceSystem::Update(*m_Registry, deltaTime);
    AssemblerSystem::Update(*m_Registry, deltaTime);
    BeltSystem::Update(*m_Registry, m_PlacementGrid, m_Settings.terrain.cellSize, deltaTime);
    BeltRenderSystem::SyncItemEntities(*m_Registry, m_VulkanEngine.get(), m_RenderSystem->GetMeshRenderer(), m_Settings.terrain.cellSize);
    InserterSystem::Update(*m_Registry, m_PlacementGrid, m_Settings.terrain.cellSize, deltaTime);
    PowerSystem::Update(*m_Registry, deltaTime);
    MoveOrderSystem::Update(*m_Registry, deltaTime, m_Settings.terrain);
    DamageSystem::ProcessDeaths(*m_Registry, glm::vec3(0.0f, 0.0f, 0.0f)); // temporary spawn point

    glm::vec3 mc = (m_Camera->GetMode() == Camera3D::Mode::IsoRTS) ? m_Camera->GetIsoTarget()
             : (m_Registry->valid(m_PlayerEntity) ? m_Registry->get<TransformComponent>(m_PlayerEntity).Position
                                                  : m_Camera->GetIsoTarget());
m_MinimapCenter = glm::vec2(mc.x, mc.z);
//
//m_MinimapTimer += deltaTime;
//if (m_MinimapTimer >= 0.25f) {
//    m_MinimapTimer = 0.0f;
//    m_MinimapTexCenter = m_MinimapCenter;
    //m_Minimap->Update(m_Fog, m_MinimapCenter, m_MinimapRange, (float)m_Settings.terrain.seed);
//}

    m_FogTimer += deltaTime;
    if (m_FogTimer >= 0.1f) {
        m_FogTimer = 0.0f;
        m_Fog.Update(*m_Registry, 0);
        glm::vec3 c = m_Camera->GetIsoTarget();
        FogTexture *fogTex = m_RenderSystem->GetFogTexture();
        if (m_Fog.ConsumeDirty() || fogTex->WindowNeedsMove(c)) {
            m_RenderSystem->GetFogTexture()->Update(m_Fog, c);
        }
    }
        

    if (m_Registry->valid(m_PlayerEntity)) {
        auto &playerTransform = m_Registry->get<TransformComponent>(m_PlayerEntity);
        glm::vec3 velocity = (playerTransform.Position - m_LastPlayerPosition) / deltaTime;
        //PlayerAnimationSystem::Update(*m_Registry, m_PlayerEntity, velocity);
        glm::vec3 loadCenter = (m_Camera->GetMode() == Camera3D::Mode::IsoRTS) ? m_Camera->GetIsoTarget() : playerTransform.Position;
        m_ChunkManager.Update(*m_Registry, loadCenter, m_Settings.terrain,
            m_VulkanEngine.get(), m_RenderSystem->GetMeshRenderer(), m_Settings.terrain.seed,
                              m_DepletionMap, m_PlacementGrid, m_RenderSystem->GetTerrainRenderer(), m_RemovedTreesMap, m_Camera->GetForwardDirection(), m_Camera->GetIsoDistance(), m_RenderSystem->GetTreeRenderer());


        m_LastPlayerPosition = playerTransform.Position;
    }

    AnimationSystem::Update(*m_Registry, deltaTime);
}

void Game::Render() {
    m_RenderSystem->RenderFrame(*m_Registry, *m_Camera, m_Settings, *m_AudioEngine,
                                m_PlayerEntity, m_InspectedEntity, m_SelectedItem, m_TechState,
                                false, m_ShowOptionsInPause, m_DayNightCycle, m_ChunkManager, m_DepletionMap, m_RemovedTreesMap);
}
void Game::Shutdown() {
    std::cout << "=== Shutting Down Game ===" << std::endl;
    m_ChunkManager.StopWorkerThread();

    std::cout << "=== Saving Settings ===" << std::endl;
    m_Settings.SaveToFile("settings.json");
    std::cout << "=== Settings Saved ===" << std::endl;
    
    if (!m_Initialized) {
		std::cout << "Game was not initialized, skipping shutdown." << std::endl;
		return;
	}  


    if (m_VulkanEngine) {
        m_VulkanEngine->WaitIdle();   
    }

    auto meshView = m_Registry->view<MeshComponent>();
    for (auto entity : meshView) {
        auto& meshComp = meshView.get<MeshComponent>(entity);
        if (meshComp.mesh) {
            meshComp.mesh->DestroyGPUResources(m_VulkanEngine->GetDevice());
        }
    }
    if (m_AudioEngine) {
        m_AudioEngine->Shutdown();
        m_AudioEngine.reset();
    }
  // if (m_Minimap)
  // {
  //     m_Minimap->Shutdown();
  // }

    if (m_Registry) {
        m_Registry->clear();
        m_Registry.reset();
    }
    if (m_RenderSystem) {
        m_RenderSystem->Shutdown();
        m_RenderSystem.reset();
    }
    if (m_VulkanEngine) {
        m_VulkanEngine->Shutdown();
        m_VulkanEngine.reset();
    }
    m_Initialized = false;
    std::cout << "=== Game Shut Down ===" << std::endl;
}

void Game::StartNewGame(const std::string &saveName) {
    m_Registry->clear();
    m_PlacementGrid = PlacementGrid{};
    m_TechState = TechState{};
    m_ChunkManager.Reset();
    CreateInitialEntities(); // existing world-gen path: player, terrain, WorldGenerator::ScatterTrees, etc.

    m_CurrentSaveName = saveName;
    SaveManager::SaveGame("Saves/" + saveName + ".json", *m_Registry, m_PlayerEntity, m_ResourceMap, m_TechState, m_Settings, m_VulkanEngine.get(), m_RenderSystem->GetMeshRenderer(), m_DepletionMap, m_RemovedTreesMap); // save immediately so the file exists

   m_ChunkManager.BeginInitialLoad(m_Registry->get<TransformComponent>(m_PlayerEntity).Position,
                                    m_Camera->GetForwardDirection(), m_Settings.terrain, m_Settings.terrain.seed);
    m_State = GameState::Loading; // CHANGED — was Playing
}

void Game::LoadExistingGame(const std::string &saveName) {
    m_Registry->clear();
    m_PlacementGrid = PlacementGrid{};
    m_ChunkManager.Reset();
    entt::entity loadedPlayer;
    bool success = SaveManager::LoadGame("Saves/" + saveName + ".json", *m_Registry, loadedPlayer,
                                         m_ResourceMap, m_PlacementGrid, m_TechState, m_Settings, m_VulkanEngine.get(), m_RenderSystem->GetMeshRenderer(), m_DepletionMap, m_RemovedTreesMap);
    if (success) {
        m_PlayerEntity = loadedPlayer;
        m_CurrentSaveName = saveName;
        m_ChunkManager.BeginInitialLoad(m_Registry->get<TransformComponent>(m_PlayerEntity).Position,
                                        m_Camera->GetForwardDirection(), m_Settings.terrain, m_Settings.terrain.seed);
        m_State = GameState::Loading; // CHANGED — was Playing
    } else {
        std::cerr << "Failed to load save: " << saveName << std::endl;
        // stay in MainMenu, maybe show an error message
    }
}

void Game::RefreshSaveList() {
    m_AvailableSaves.clear();
    if (!std::filesystem::exists("Saves")) {
        std::filesystem::create_directories("Saves");
        return;
    }
    for (auto &entry : std::filesystem::directory_iterator("Saves")) {
        if (entry.path().extension() == ".json") {
            m_AvailableSaves.push_back(entry.path().stem().string());
        }
    }
}

void Game::RunMainMenu() {
    if (!m_VulkanEngine->BeginFrame()) {
        return; // swapchain recreation in progress — skip this frame entirely, ImGui::NewFrame was never called so nothing to close
    }
    m_RenderSystem->GetImGuiUtil()->NewFrame(); // adjust to however you drive ImGui's frame outside RenderFrame's normal path

    ImGuiIO &io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 350));
    ImGui::Begin("Main Menu", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

    ImGui::Text("My Factory Game");
    ImGui::Separator();

    ImGui::InputText("World Name", m_NewGameNameBuffer, sizeof(m_NewGameNameBuffer));
    if (ImGui::Button("New Game", ImVec2(-1, 40))) {
        StartNewGame(m_NewGameNameBuffer);
    }

    ImGui::Separator();
    ImGui::Text("Load Game:");
    RefreshSaveList();
    for (auto &save : m_AvailableSaves) {
        if (ImGui::Button(save.c_str(), ImVec2(-1, 0))) {
            LoadExistingGame(save);
        }
    }

    ImGui::Separator();
    ImGui::BeginDisabled(true);
    ImGui::Button("Multiplayer (coming soon)", ImVec2(-1, 40));
    ImGui::EndDisabled();

    if (ImGui::Button("Options", ImVec2(-1, 40))) {
        m_ShowOptionsInMenu = true;
    }

    if (ImGui::Button("Quit", ImVec2(-1, 40))) {
        m_IsRunning = false;
    }

    ImGui::End();

    if (m_ShowOptionsInMenu) {
        ImGui::Begin("Options", &m_ShowOptionsInMenu);
        // reuse your existing settings sliders from DrawSettingsTab here
        ImGui::End();
    }

    // Render just the menu — no 3D scene, no BeginFrame's mesh/quad rendering needed

    m_RenderSystem->GetImGuiUtil()->RenderDrawData(m_VulkanEngine->GetCurrentCommandBuffer());
    m_VulkanEngine->EndFrame();
}
void Game::RunPauseMenu() {
    PauseMenuAction action = m_RenderSystem->RenderFrame(*m_Registry, *m_Camera, m_Settings, *m_AudioEngine,
                                                         m_PlayerEntity, m_InspectedEntity, m_SelectedItem, m_TechState,
                                                         true, m_ShowOptionsInPause, m_DayNightCycle,m_ChunkManager,m_DepletionMap, m_RemovedTreesMap);

    switch (action) {
    case PauseMenuAction::Resume:
        m_State = GameState::Playing;
        break;
    case PauseMenuAction::SaveGame:
        SaveManager::SaveGame("Saves/" + m_CurrentSaveName + ".json", *m_Registry, m_PlayerEntity, m_ResourceMap, m_TechState, m_Settings, m_VulkanEngine.get(), m_RenderSystem->GetMeshRenderer(), m_DepletionMap, m_RemovedTreesMap);
        break;
    case PauseMenuAction::QuitToMenu:
        m_Registry->clear();
        m_PlacementGrid = PlacementGrid{};
        m_DepletionMap = DepletionMap{};
        m_RemovedTreesMap = RemovedTreesMap{};
        m_State = GameState::MainMenu;
        break;
    case PauseMenuAction::QuitGame:
        m_IsRunning = false;
        break;
    default:
        break;
    }
}
void Game::RunLoadingScreen() {
    // Drive chunk loading during this state too — this replaces the old blocking/async CPU approach
    if (m_Registry->valid(m_PlayerEntity)) {
        auto& playerTransform = m_Registry->get<TransformComponent>(m_PlayerEntity);
        m_ChunkManager.Update(*m_Registry, playerTransform.Position, m_Settings.terrain,
            m_VulkanEngine.get(), m_RenderSystem->GetMeshRenderer(), m_Settings.terrain.seed,
            m_DepletionMap, m_PlacementGrid, m_RenderSystem->GetTerrainRenderer(), m_RemovedTreesMap, m_Camera->GetForwardDirection(), m_Camera->GetIsoDistance(), m_RenderSystem->GetTreeRenderer());
    }

    if (!m_VulkanEngine->BeginFrame([&](VkCommandBuffer cmd) {
        std::cout << "About to call RecordPendingGeneration Game.cpp" << std::endl;
        m_ChunkManager.RecordPendingGeneration(cmd, m_RenderSystem->GetChunkGenerator(), m_RenderSystem->GetTerrainRenderer(), m_RenderSystem->GetTreeRenderer(), m_Settings.terrain, m_DepletionMap, m_RemovedTreesMap);
        std::cout << "RecordPendingGeneration complete Game.cpp" << std::endl;
        })) {
        return;
    }

    m_RenderSystem->GetImGuiUtil()->NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 100));
    ImGui::Begin("Loading", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);

    ImGui::Text("Generating world...");
    float progress = m_ChunkManager.GetInitialLoadProgress();
    ImGui::ProgressBar(progress, ImVec2(-1, 30));

    ImGui::End();

    m_RenderSystem->GetImGuiUtil()->RenderDrawData(m_VulkanEngine->GetCurrentCommandBuffer());
    m_VulkanEngine->EndFrame();
}

void Game::RunNoiseLibraryTest()
{
    VkDevice device = m_VulkanEngine->GetDevice();

    const uint32_t sampleCount = 64;

    // Output buffer — host-visible so we can read it back directly
    VkBuffer outputBuffer;
    VkDeviceMemory outputMemory;
    VkDeviceSize bufferSize = sampleCount * sizeof(float);
    m_VulkanEngine->CreateBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        outputBuffer, outputMemory);
    void* mapped;
    vkMapMemory(device, outputMemory, 0, bufferSize, 0, &mapped);

    // Descriptor set layout: one storage buffer
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;
    VkDescriptorSetLayout descSetLayout;
    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descSetLayout);

    VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 };
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;
    VkDescriptorPool descPool;
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &descPool);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descSetLayout;
    VkDescriptorSet descSet;
    vkAllocateDescriptorSets(device, &allocInfo, &descSet);

    VkDescriptorBufferInfo bufferInfo{ outputBuffer, 0, bufferSize };
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descSet;
    write.dstBinding = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.descriptorCount = 1;
    write.pBufferInfo = &bufferInfo;
    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

    // Pipeline layout with push constants
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(NoiseTestPush);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushRange;
    VkPipelineLayout pipelineLayout;
    vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);

    auto shaderCode = m_VulkanEngine->ReadFile("Shaders/noise_test_comp.spv");
    VkShaderModule shaderModule = m_VulkanEngine->CreateShaderModule(shaderCode);

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = shaderModule;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = stageInfo;
    pipelineInfo.layout = pipelineLayout;

    VkPipeline pipeline;
    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create noise test compute pipeline");
    }
    vkDestroyShaderModule(device, shaderModule, nullptr);

    // Run once per sample type
    const char* typeNames[] = { "simplex", "fbm", "ridged" };
    for (uint32_t sampleType = 0; sampleType < 3; sampleType++) {
        NoiseTestPush push{ 0.0f, 0.0f, 1.0f, 1 };  // CHANGED — spacing=15, spans -500 to +460 across 64 samples

        VkCommandBuffer cmd = m_VulkanEngine->BeginSingleTimeCommands();
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descSet, 0, nullptr);
        vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(NoiseTestPush), &push);
        vkCmdDispatch(cmd, sampleCount / 64, 1, 1);
        m_VulkanEngine->EndSingleTimeCommands(cmd);

        float* results = static_cast<float*>(mapped);
        float minVal = results[0], maxVal = results[0];
        for (uint32_t i = 0; i < sampleCount; i++) {
            minVal = std::min(minVal, results[i]);
            maxVal = std::max(maxVal, results[i]);
        }
        std::cout << "Noise test [" << typeNames[sampleType] << "]: min=" << minVal << " max=" << maxVal << " | first 8: ";
        for (int i = 0; i < 64; i++) std::cout << results[i] << " ";
        std::cout << std::endl;
    }

    // Cleanup
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorPool(device, descPool, nullptr);
    vkDestroyDescriptorSetLayout(device, descSetLayout, nullptr);
    vkDestroyBuffer(device, outputBuffer, nullptr);
    vkFreeMemory(device, outputMemory, nullptr);
    //Test
}

// Game.cpp
void Game::RunBiomeLibraryTest() {
    VkDevice device = m_VulkanEngine->GetDevice();

    const uint32_t sampleCount = 1024;

    VkBuffer outputBuffer;
    VkDeviceMemory outputMemory;
    VkDeviceSize bufferSize = sampleCount * sizeof(float);
    m_VulkanEngine->CreateBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        outputBuffer, outputMemory);
    void* mapped;
    vkMapMemory(device, outputMemory, 0, bufferSize, 0, &mapped);

    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;
    VkDescriptorSetLayout descSetLayout;
    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descSetLayout);

    VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 };
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;
    VkDescriptorPool descPool;
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &descPool);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descSetLayout;
    VkDescriptorSet descSet;
    vkAllocateDescriptorSets(device, &allocInfo, &descSet);

    VkDescriptorBufferInfo bufferInfo{ outputBuffer, 0, bufferSize };
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descSet;
    write.dstBinding = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.descriptorCount = 1;
    write.pBufferInfo = &bufferInfo;
    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(BiomeTestPush);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushRange;
    VkPipelineLayout pipelineLayout;
    vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);

    auto shaderCode = m_VulkanEngine->ReadFile("Shaders/biome_test_comp.spv");
    VkShaderModule shaderModule = m_VulkanEngine->CreateShaderModule(shaderCode);

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = shaderModule;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = stageInfo;
    pipelineInfo.layout = pipelineLayout;

    VkPipeline pipeline;
    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create biome test compute pipeline");
    }
    vkDestroyShaderModule(device, shaderModule, nullptr);

    BiomeTestPush push{ -32.0f, 312.0f, 0.0f, (float)m_Settings.terrain.seed, 1, 0.0f };

    VkCommandBuffer cmd = m_VulkanEngine->BeginSingleTimeCommands();
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descSet, 0, nullptr);
    vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BiomeTestPush), &push);
    vkCmdDispatch(cmd, sampleCount / 64, 1, 1);
    m_VulkanEngine->EndSingleTimeCommands(cmd);

    float* results = static_cast<float*>(mapped);
    std::cout << "GPU elevationTrigger at (-32,312) [single point test] = " << results[0] << std::endl;

    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorPool(device, descPool, nullptr);
    vkDestroyDescriptorSetLayout(device, descSetLayout, nullptr);
    vkDestroyBuffer(device, outputBuffer, nullptr);
    vkFreeMemory(device, outputMemory, nullptr);
}
void Game::RunBiomeGridTest() {
    VkDevice device = m_VulkanEngine->GetDevice();

    const uint32_t gridSize = 128;   // 128x128 = 16,384 samples total
    const uint32_t sampleCount = gridSize * gridSize;

    VkBuffer outputBuffer;
    VkDeviceMemory outputMemory;
    VkDeviceSize bufferSize = sampleCount * sizeof(float);
    m_VulkanEngine->CreateBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        outputBuffer, outputMemory);
    void* mapped;
    vkMapMemory(device, outputMemory, 0, bufferSize, 0, &mapped);

    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;
    VkDescriptorSetLayout descSetLayout;
    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descSetLayout);

    VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 };
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;
    VkDescriptorPool descPool;
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &descPool);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descSetLayout;
    VkDescriptorSet descSet;
    vkAllocateDescriptorSets(device, &allocInfo, &descSet);

    VkDescriptorBufferInfo bufferInfo{ outputBuffer, 0, bufferSize };
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descSet;
    write.dstBinding = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.descriptorCount = 1;
    write.pBufferInfo = &bufferInfo;
    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

    struct GridPush {
        float startX, startZ, spacing, seed;   // CHANGED — worldExtentZ removed
        uint32_t gridSizeVal;
    };

    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(GridPush);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushRange;
    VkPipelineLayout pipelineLayout;
    vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);

    auto shaderCode = m_VulkanEngine->ReadFile("Shaders/biome_grid_test_comp.spv");
    VkShaderModule shaderModule = m_VulkanEngine->CreateShaderModule(shaderCode);

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = shaderModule;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = stageInfo;
    pipelineInfo.layout = pipelineLayout;

    VkPipeline pipeline;
    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create biome grid test compute pipeline");
    }
    vkDestroyShaderModule(device, shaderModule, nullptr);

    float spacing = 20.0f;   // CHANGED — wider spacing to cover more real distance given the much larger latitude band
    float rangeSpan = gridSize * spacing;   // 128 * 20 = 2560 units per axis — comparable to LATITUDE_BAND_SIZE
    float startCoord = -(rangeSpan * 0.5f);   // centered on world origin (the new fixed equator)

    GridPush push{ startCoord, startCoord, spacing, (float)m_Settings.terrain.seed, gridSize };

    VkCommandBuffer cmd = m_VulkanEngine->BeginSingleTimeCommands();
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descSet, 0, nullptr);
    vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(GridPush), &push);
    vkCmdDispatch(cmd, (gridSize + 7) / 8, (gridSize + 7) / 8, 1);   // matches local_size_x/y = 8
    m_VulkanEngine->EndSingleTimeCommands(cmd);

    float* results = static_cast<float*>(mapped);
    int biomeCounts[6] = { 0, 0, 0, 0, 0, 0 };
    for (uint32_t i = 0; i < sampleCount; i++) {
        int biomeId = static_cast<int>(results[i]);
        if (biomeId >= 0 && biomeId < 6) biomeCounts[biomeId]++;
    }

    const char* biomeNames[] = { "Plains", "Desert", "Tundra", "Hills", "Mountains", "Lake" };
    std::cout << "=== 2D Biome Grid Test (" << gridSize << "x" << gridSize << " = " << sampleCount << " samples) ===" << std::endl;
    for (int b = 0; b < 6; b++) {
        std::cout << "  " << biomeNames[b] << ": " << biomeCounts[b] << " (" << (100.0f * biomeCounts[b] / sampleCount) << "%)" << std::endl;
    }

    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorPool(device, descPool, nullptr);
    vkDestroyDescriptorSetLayout(device, descSetLayout, nullptr);
    vkDestroyBuffer(device, outputBuffer, nullptr);
    vkFreeMemory(device, outputMemory, nullptr);
}

void Game::RunSimplexDebugTest(glm::vec2 testPoint) {
    VkDevice device = m_VulkanEngine->GetDevice();

    const uint32_t numVec4s = 9;   // CHANGED — was 8
    VkBuffer outputBuffer;
    VkDeviceMemory outputMemory;
    VkDeviceSize bufferSize = numVec4s * sizeof(glm::vec4);
    m_VulkanEngine->CreateBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        outputBuffer, outputMemory);
    void* mapped;
    vkMapMemory(device, outputMemory, 0, bufferSize, 0, &mapped);

    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;
    VkDescriptorSetLayout descSetLayout;
    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descSetLayout);

    VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 };
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;
    VkDescriptorPool descPool;
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &descPool);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descSetLayout;
    VkDescriptorSet descSet;
    vkAllocateDescriptorSets(device, &allocInfo, &descSet);

    VkDescriptorBufferInfo bufferInfo{ outputBuffer, 0, bufferSize };
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descSet;
    write.dstBinding = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.descriptorCount = 1;
    write.pBufferInfo = &bufferInfo;
    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(glm::vec2);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushRange;
    VkPipelineLayout pipelineLayout;
    vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);

    auto shaderCode = m_VulkanEngine->ReadFile("Shaders/simplex_debug_comp.spv");
    VkShaderModule shaderModule = m_VulkanEngine->CreateShaderModule(shaderCode);

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = shaderModule;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = stageInfo;
    pipelineInfo.layout = pipelineLayout;

    VkPipeline pipeline;
    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create simplex debug pipeline");
    }
    vkDestroyShaderModule(device, shaderModule, nullptr);

    VkCommandBuffer cmd = m_VulkanEngine->BeginSingleTimeCommands();
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descSet, 0, nullptr);
    vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(glm::vec2), &testPoint);
    vkCmdDispatch(cmd, 1, 1, 1);
    m_VulkanEngine->EndSingleTimeCommands(cmd);

    glm::vec4* results = static_cast<glm::vec4*>(mapped);
    const char* labels[9] = { "i", "x0", "i1", "x12", "p", "mPreSquare", "m", "g", "result" };   // CHANGED
    for (int i = 0; i < 9; i++) {   // CHANGED
        std::cout << "  " << labels[i] << " = (" << results[i].x << ", " << results[i].y << ", " << results[i].z << ", " << results[i].w << ")" << std::endl;
    }

    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorPool(device, descPool, nullptr);
    vkDestroyDescriptorSetLayout(device, descSetLayout, nullptr);
    vkDestroyBuffer(device, outputBuffer, nullptr);
    vkFreeMemory(device, outputMemory, nullptr);
}

void Game::RunRidgedDebugTest(glm::vec2 pos, int octaves, float frequency, float lacunarity, float gain) {
    VkDevice device = m_VulkanEngine->GetDevice();

    const uint32_t numVec4s = 4;
    VkBuffer outputBuffer;
    VkDeviceMemory outputMemory;
    VkDeviceSize bufferSize = numVec4s * sizeof(glm::vec4);
    m_VulkanEngine->CreateBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        outputBuffer, outputMemory);
    void* mapped;
    vkMapMemory(device, outputMemory, 0, bufferSize, 0, &mapped);

    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;
    VkDescriptorSetLayout descSetLayout;
    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descSetLayout);

    VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 };
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;
    VkDescriptorPool descPool;
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &descPool);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descSetLayout;
    VkDescriptorSet descSet;
    vkAllocateDescriptorSets(device, &allocInfo, &descSet);

    VkDescriptorBufferInfo bufferInfo{ outputBuffer, 0, bufferSize };
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descSet;
    write.dstBinding = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.descriptorCount = 1;
    write.pBufferInfo = &bufferInfo;
    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

    struct RidgedPush {
        glm::vec2 pos;
        int octaves;
        float frequency;
        float lacunarity;
        float gain;
    };
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(RidgedPush);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushRange;
    VkPipelineLayout pipelineLayout;
    vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);

    auto shaderCode = m_VulkanEngine->ReadFile("Shaders/ridged_debug_comp.spv");
    VkShaderModule shaderModule = m_VulkanEngine->CreateShaderModule(shaderCode);

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = shaderModule;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = stageInfo;
    pipelineInfo.layout = pipelineLayout;

    VkPipeline pipeline;
    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create ridged debug pipeline");
    }
    vkDestroyShaderModule(device, shaderModule, nullptr);

    RidgedPush push{ pos, octaves, frequency, lacunarity, gain };

    VkCommandBuffer cmd = m_VulkanEngine->BeginSingleTimeCommands();
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descSet, 0, nullptr);
    vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(RidgedPush), &push);
    vkCmdDispatch(cmd, 1, 1, 1);
    m_VulkanEngine->EndSingleTimeCommands(cmd);

    glm::vec4* results = static_cast<glm::vec4*>(mapped);
    std::cout << "=== GPU RidgedNoise octaves at (" << pos.x << "," << pos.y << ") ===" << std::endl;
    for (int i = 0; i < 3; i++) {
        std::cout << "  octave " << i << ": sum=" << results[i].x << " n=" << results[i].y
            << " amplitude=" << results[i].z << " freq=" << results[i].w << std::endl;
    }
    std::cout << "  final result=" << results[3].x << std::endl;

    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorPool(device, descPool, nullptr);
    vkDestroyDescriptorSetLayout(device, descSetLayout, nullptr);
    vkDestroyBuffer(device, outputBuffer, nullptr);
    vkFreeMemory(device, outputMemory, nullptr);
}


void Game::CompareTerrainHeightAt(float x, float z) {
    VkDevice device = m_VulkanEngine->GetDevice();
    const uint32_t count = 64;                       // shader is local_size_x = 64
    VkDeviceSize size = count * sizeof(float);

    VkBuffer buf; VkDeviceMemory mem;
    m_VulkanEngine->CreateBuffer(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buf, mem);
    void* mapped; vkMapMemory(device, mem, 0, size, 0, &mapped);

    VkDescriptorSetLayoutBinding b{}; b.binding = 0;
    b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; b.descriptorCount = 1;
    b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    VkDescriptorSetLayoutCreateInfo li{}; li.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    li.bindingCount = 1; li.pBindings = &b;
    VkDescriptorSetLayout dsl; vkCreateDescriptorSetLayout(device, &li, nullptr, &dsl);

    VkDescriptorPoolSize ps{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 };
    VkDescriptorPoolCreateInfo pi{}; pi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pi.poolSizeCount = 1; pi.pPoolSizes = &ps; pi.maxSets = 1;
    VkDescriptorPool pool; vkCreateDescriptorPool(device, &pi, nullptr, &pool);

    VkDescriptorSetAllocateInfo ai{}; ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool = pool; ai.descriptorSetCount = 1; ai.pSetLayouts = &dsl;
    VkDescriptorSet set; vkAllocateDescriptorSets(device, &ai, &set);

    VkDescriptorBufferInfo bi{ buf, 0, size };
    VkWriteDescriptorSet w{}; w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w.dstSet = set; w.dstBinding = 0; w.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    w.descriptorCount = 1; w.pBufferInfo = &bi;
    vkUpdateDescriptorSets(device, 1, &w, 0, nullptr);



    VkPushConstantRange pr{ VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BiomeTestPush) };
    VkPipelineLayoutCreateInfo pli{}; pli.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pli.setLayoutCount = 1; pli.pSetLayouts = &dsl;
    pli.pushConstantRangeCount = 1; pli.pPushConstantRanges = &pr;
    VkPipelineLayout layout; vkCreatePipelineLayout(device, &pli, nullptr, &layout);

    auto code = m_VulkanEngine->ReadFile("Shaders/biome_test_comp.spv");
    VkShaderModule mod = m_VulkanEngine->CreateShaderModule(code);
    VkPipelineShaderStageCreateInfo st{}; st.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    st.stage = VK_SHADER_STAGE_COMPUTE_BIT; st.module = mod; st.pName = "main";
    VkComputePipelineCreateInfo ci{}; ci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    ci.stage = st; ci.layout = layout;
    VkPipeline pipe; vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &ci, nullptr, &pipe);
    vkDestroyShaderModule(device, mod, nullptr);

    // sampleType 4 = terrain height; spacing 0 so every thread samples the same point
    BiomeTestPush push{ x, z, 0.0f, (float)m_Settings.terrain.seed, 5, 0.0f };

    VkCommandBuffer cmd = m_VulkanEngine->BeginSingleTimeCommands();
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipe);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1, &set, 0, nullptr);
    vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BiomeTestPush), &push);
    vkCmdDispatch(cmd, 1, 1, 1);
    m_VulkanEngine->EndSingleTimeCommands(cmd);


    float* g = static_cast<float*>(mapped);
    float s = (float)m_Settings.terrain.seed;
    std::cout << "elev   GPU=" << g[0] << " CPU=" << GpuNoiseMatch::GetElevationTrigger(x, z, s) << "\n"
        << "hills  GPU=" << g[1] << " CPU=" << GpuNoiseMatch::SampleRawHeightForBiome(x, z, BIOME_HILLS, s) << "\n"
        << "mtns   GPU=" << g[2] << " CPU=" << GpuNoiseMatch::SampleRawHeightForBiome(x, z, BIOME_MOUNTAINS, s) << "\n"
        << "plains GPU=" << g[3] << " CPU=" << GpuNoiseMatch::SampleRawHeightForBiome(x, z, BIOME_PLAINS, s) << "\n"
        << "biome  GPU=" << g[4] << " CPU=" << GpuNoiseMatch::DetermineBiome(x, z, s) << "\n"
        << "arid   GPU=" << g[5] << " CPU=" << GpuNoiseMatch::GetAridity(x, z, s) << "\n"
        << "hashSeed GPU=" << g[6] << " CPU=" << GpuNoiseMatch::HashSeed(s) << "\n"
        << "  seed GPU=" << g[7] << " CPU=" << s << std::endl;
/*    float gpu = static_cast<float*>(mapped)[0];
    float cpu = GpuNoiseMatch::SampleHeight(x, z, (float)m_Settings.terrain.seed);
    std::cout << "(" << x << "," << z << ") GPU=" << gpu << " CPU=" << cpu
        << " diff=" << (gpu - cpu) << std::endl;
        */
    vkDestroyPipeline(device, pipe, nullptr);
    vkDestroyPipelineLayout(device, layout, nullptr);
    vkDestroyDescriptorPool(device, pool, nullptr);
    vkDestroyDescriptorSetLayout(device, dsl, nullptr);
    vkDestroyBuffer(device, buf, nullptr);
    vkFreeMemory(device, mem, nullptr);
}

void Game::DrawMinimap() {
    ImGuiIO &io = ImGui::GetIO();
    const float size = 220.0f;

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 10.0f, io.DisplaySize.y - 10.0f), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
    ImGui::SetNextWindowSize(ImVec2(size, size));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Minimap", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);

    ImGui::Image((ImTextureID)(uintptr_t)m_Minimap.GetImGuiTexture(), ImVec2(size, size));
    ImVec2 imgMin = ImGui::GetItemRectMin();
    bool hovered = ImGui::IsItemHovered();

    // world <-> minimap helpers, using the center the texture was drawn with
    auto worldToScreen = [&](float wx, float wz) {
        float u = (wx - GetMinimapCenter().x) / (m_MinimapRange * 2.0f) + 0.5f;
        float v = (wz - GetMinimapCenter().y) / (m_MinimapRange * 2.0f) + 0.5f;
        return ImVec2(imgMin.x + u * size, imgMin.y + v * size);
    };

    ImDrawList *dl = ImGui::GetWindowDrawList();
    dl->AddRect(imgMin, ImVec2(imgMin.x + size, imgMin.y + size), IM_COL32(200, 200, 200, 255));

    // unit dots: own team always, other teams only where currently visible
    auto units = m_Registry->view<TransformComponent, TeamComponent>();
    for (auto e : units) {
        glm::vec3 p = units.get<TransformComponent>(e).Position;
        uint8_t team = units.get<TeamComponent>(e).team;
        if (team != 0 && m_Fog.GetState(p.x, p.z) != FogOfWar::Visible) continue;
        ImVec2 s = worldToScreen(p.x, p.z);
        if (s.x < imgMin.x || s.y < imgMin.y || s.x > imgMin.x + size || s.y > imgMin.y + size) continue;
        dl->AddCircleFilled(s, 3.0f, team == 0 ? IM_COL32(80, 255, 120, 255) : IM_COL32(255, 70, 70, 255));
    }

    auto placed = m_Registry->view<TransformComponent, BoundsComponent>(entt::exclude<SelectableComponent, PickupComponent>);
    float pxPerUnit = size / (m_MinimapRange * 2.0f);
    for (auto e : placed) {
        glm::vec3 p = placed.get<TransformComponent>(e).Position;
        if (m_Fog.GetState(p.x, p.z) == FogOfWar::Unexplored) continue;

        glm::vec3 he = placed.get<BoundsComponent>(e).halfExtents;
        ImVec2 s = worldToScreen(p.x, p.z);
        float hx = std::max(he.x * pxPerUnit, 1.5f);
        float hz = std::max(he.z * pxPerUnit, 1.5f);
        if (s.x + hx < imgMin.x || s.y + hz < imgMin.y || s.x - hx > imgMin.x + size || s.y - hz > imgMin.y + size) continue;

        dl->AddRectFilled(ImVec2(s.x - hx, s.y - hz), ImVec2(s.x + hx, s.y + hz), IM_COL32(240, 170, 60, 255));
    }


    // camera marker
    glm::vec3 cam = m_Camera->GetIsoTarget();
    ImVec2 cs = worldToScreen(cam.x, cam.z);
    dl->AddRect(ImVec2(cs.x - 5, cs.y - 5), ImVec2(cs.x + 5, cs.y + 5), IM_COL32(255, 255, 255, 255));

    // input
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        m_MinimapDragCenter = m_MinimapCenter; // lock before anything moves
        m_MinimapDragging = true;
    }
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) m_MinimapDragging = false;

    if (hovered && io.MouseWheel != 0.0f)
        m_MinimapRange = glm::clamp(m_MinimapRange * (io.MouseWheel > 0 ? 0.85f : 1.18f), 50.0f, 2000.0f);

    float u = (io.MousePos.x - imgMin.x) / size;
    float v = (io.MousePos.y - imgMin.y) / size;
    u = glm::clamp(u, 0.0f, 1.0f); // keep dragging inside the map edges
    v = glm::clamp(v, 0.0f, 1.0f);
    glm::vec2 world = GetMinimapCenter() + glm::vec2(u - 0.5f, v - 0.5f) * (m_MinimapRange * 2.0f);

    if (m_MinimapDragging) {
        if (m_Camera->GetMode() != Camera3D::Mode::IsoRTS)
            m_Camera->SetMode(Camera3D::Mode::IsoRTS);
        m_Camera->SetIsoTarget(glm::vec3(world.x, 0.0f, world.y));
        std::cout << "u=" << u << " v=" << v << " center=" << GetMinimapCenter().x << "," << GetMinimapCenter().y
                  << " range=" << m_MinimapRange << " world=" << world.x << "," << world.y << std::endl;
    }

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !m_SelectedUnits.empty()) {
        float h = GpuNoiseMatch::SampleHeight(world.x, world.y, (float)m_Settings.terrain.seed);
        IssueMoveCommand(glm::vec3(world.x, h, world.y));
    }

    ImGui::End();
    ImGui::PopStyleVar();
}