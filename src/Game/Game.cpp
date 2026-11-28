#include "Game.h"
#include <filesystem>
#include <iostream>
#include <random>
#include <imgui/imgui.h>
#include "../Engine/VulkanEngine.h"
#include "../Renderer/RenderSystem.h"
#include "../Renderer/ImGuiVulkanUtil.h"
#include "../Renderer/TreeRenderer.h"
#include "../Rendering/ModelLoader.h"
#include "../Rendering/SkinnedMesh.h"
#include "../Rendering/FogTexture.h"
#include "../Components/Components.h"
#include "../Helpers/SaveManager.h"
#include "../Helpers/PauseMenuAction.h"
#include "../World/TerrainRaycast.h"
#include "../World/WorldGenerator.h"
#include "../Utils/GpuNoiseMatch.h"
#include "../Systems/InteractionSystem.h"
#include "../Systems/PlacementSystem.h"
#include "../Systems/MinerSystem.h"
#include "../Systems/FurnaceSystem.h"
#include "../Systems/AssemblerSystem.h"
#include "../Systems/BeltSystem.h"
#include "../Systems/BeltRenderSystem.h"
#include "../Systems/InserterSystem.h"
#include "../Systems/MoveOrderSystem.h"
#include "../Systems/PowerSystem.h"
#include "../Systems/AnimationSystem.h"
#include "../Systems/DamageSystem.h"
#include "../Systems/CombatSystem.h"
#include "../Systems/VictorySystem.h"
#include "Camera3D.h"
#include "PlaceableDatabase.h"
#include "RecipeDatabase.h"
#include "FurnaceRecipeDatabase.h"
#include "FuelDatabase.h"
#include "UnitDatabase.h"
#include "EntityFactory.h"

namespace {
    constexpr float INTERACT_RANGE = 25.0f;
    constexpr float MACHINE_PICK_RANGE = 100.0f;
    constexpr float UNIT_PICK_RANGE = 500.0f;
    constexpr float PICKUP_RADIUS = 8.0f;
    constexpr int UNIT_CAP = 200;
    constexpr float FOG_UPDATE_INTERVAL = 0.1f;
    constexpr float CLICK_DRAG_THRESHOLD = 5.0f; // pixels; below this a box-select counts as a click
    using EntityFactory::PLAYER_TEAM;
}

Game::Game() = default;

Game::~Game() {
    Shutdown();
}

// ============================================================================
// Main loop
// ============================================================================

void Game::Run() {
    Init();
    m_IsRunning = true;
    float lastTime = static_cast<float>(glfwGetTime());

    while (m_IsRunning && !m_VulkanEngine->ShouldClose()) {
        m_VulkanEngine->PollEvents();
        m_Input.Update(m_VulkanEngine->GetWindow());

        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        if (m_Input.Pressed(GLFW_KEY_ESCAPE)) {
            if (m_State == GameState::Playing)
                m_State = GameState::Paused;
            else if (m_State == GameState::Paused)
                m_State = GameState::Playing;
        }

        switch (m_State) {
        case GameState::MainMenu:
            RunMainMenu();
            break;

        case GameState::Playing:
            if (m_PendingLoad) {
                m_PendingLoad = false;
                if (!LoadSave(m_CurrentSaveName)) {
                    std::cerr << "Failed to reload save: " << m_CurrentSaveName << std::endl;
                    m_State = GameState::MainMenu; // world was already cleared, nothing to return to
                    break;
                }
                m_ChunkManager.Reset();
                BeginChunkLoad();
                break;
            }
            if (m_PendingMenu) {
                m_PendingMenu = false;
                ResetWorldState();
                m_State = GameState::MainMenu;
                break;
            }
            HandleInput(deltaTime);
            Update(deltaTime);
            Render();
            break;

        case GameState::Paused:
            RunPauseMenu(); // renders the frozen last frame + pause UI on top, no gameplay update
            break;

        case GameState::Loading:
            RunLoadingScreen();
            if (m_ChunkManager.IsInitialLoadComplete())
                m_State = GameState::Playing;
            break;
        }
    }

    Shutdown();
}

void Game::Init() {
    std::cout << "=== Loading Settings ===" << std::endl;
    m_Settings = GameSettings::LoadFromFile("settings.json");

    std::cout << "=== Registering Databases ===" << std::endl;
    ItemDatabase::Init();
    UnitDatabase::Init();
    RecipeDatabase::Init();
    FuelDatabase::Init();
    PlaceableDatabase::Init();
    FurnaceRecipeDatabase::Init();

    m_PlacementSystem = std::make_unique<PlacementSystem>();

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

    // MAX_ZOOM_FACTOR must match ChunkManager::Update's zoom scaling exactly
    int scaledRadius = static_cast<int>(ChunkManager::LOAD_RADIUS_CHUNKS * ChunkManager::MAX_ZOOM_FACTOR);
    uint32_t maxChunks = (scaledRadius * 2 + 1) * (scaledRadius * 2 + 1) + 200; // full worst-case square + buffer

    m_RenderSystem->Init(vertsPerChunk, indicesPerChunk, maxChunks);
    m_Camera = std::make_unique<Camera3D>(glm::vec3(0.0f, 5.0f, 5.0f));

    m_VulkanEngine->ChainScrollCallback();
    m_HUD.Init(m_VulkanEngine.get(), m_RenderSystem->GetFogTexture());
    m_RenderSystem->SetPreRenderCallback([this](VkCommandBuffer cmd) { m_HUD.RecordMinimap(cmd, m_Settings.terrain.seed); });
    m_RenderSystem->SetUICallback([this]() { DrawHUD(); });

    m_Registry = std::make_unique<entt::registry>();
    m_UnitMesh = std::make_shared<SkinnedMesh>(ModelLoader::LoadSkinnedModel(
        EntityFactory::UNIT_MODEL_PATH, m_VulkanEngine.get(), m_RenderSystem->GetMeshRenderer()));

    m_Initialized = true;
    std::cout << "=== Engine Initialized ===" << std::endl;
}

void Game::Shutdown() {
    std::cout << "=== Shutting Down Game ===" << std::endl;
    m_Settings.SaveToFile("settings.json");

    if (!m_Initialized) return;

    if (m_VulkanEngine) m_VulkanEngine->WaitIdle();

    auto meshView = m_Registry->view<MeshComponent>();
    for (auto entity : meshView) {
        auto &meshComp = meshView.get<MeshComponent>(entity);
        if (meshComp.mesh) meshComp.mesh->DestroyGPUResources(m_VulkanEngine->GetDevice());
    }
    if (m_AudioEngine) {
        m_AudioEngine->Shutdown();
        m_AudioEngine.reset();
    }
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

// ============================================================================
// Input
// ============================================================================

Game::MouseRay Game::GetMouseRay() const {
    GLFWwindow *window = m_VulkanEngine->GetWindow();
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    int winW, winH;
    glfwGetWindowSize(window, &winW, &winH);                  // logical size
    VkExtent2D extent = m_VulkanEngine->GetSwapChainExtent(); // physical size (framebuffer)

    MouseRay ray;
    ray.screen = glm::vec2((float)extent.width, (float)extent.height);
    // Convert logical cursor position to physical pixels (DPI scaling)
    ray.cursor = glm::vec2((float)mx * (winW > 0 ? ray.screen.x / winW : 1.0f),
                           (float)my * (winH > 0 ? ray.screen.y / winH : 1.0f));
    ray.aspect = ray.screen.x / ray.screen.y;
    ray.origin = m_Camera->GetEyePosition();
    ray.dir = m_Camera->ScreenPointToRay(ray.cursor.x, ray.cursor.y, ray.screen.x, ray.screen.y, ray.aspect);
    return ray;
}

float Game::GetAspect() const {
    VkExtent2D extent = m_VulkanEngine->GetSwapChainExtent();
    return static_cast<float>(extent.width) / static_cast<float>(extent.height);
}

void Game::HandleInput(float deltaTime) {
    if (m_Input.Pressed(GLFW_KEY_F2)) {
        bool nowIso = m_Camera->GetMode() == Camera3D::Mode::Isometric;
        m_Camera->SetMode(nowIso ? Camera3D::Mode::FreeFly : Camera3D::Mode::Isometric);
        m_FirstMouse = true; // avoid a jump from mouse movement while the other mode was active
    }
    if (m_Input.Pressed(GLFW_KEY_F11)) m_VulkanEngine->ToggleFullscreen();

    if (m_Camera->GetMode() == Camera3D::Mode::FreeFly)
        HandleFreeFlyInput(deltaTime);
    else
        HandleIsoInput(deltaTime);
}

void Game::HandleIsoInput(float deltaTime) {
    const MouseRay ray = GetMouseRay();
    const bool mouseOverUI = ImGui::GetIO().WantCaptureMouse;
    const bool rtsMode = m_Camera->GetMode() == Camera3D::Mode::IsoRTS;

    HandleDebugInput(ray);

    entt::entity machineTarget = InteractionSystem::FindMachineAlongRay(*m_Registry, ray.origin, ray.dir, MACHINE_PICK_RANGE);
    bool hasMachine = m_Registry->valid(machineTarget);

    // Building / machines
    if (m_Input.Pressed(GLFW_KEY_G))
        m_PlacementSystem->TryConfirmPlacement(*m_Registry, m_PlayerEntity, m_World.placementGrid, m_Settings.terrain);
    if (m_Input.Pressed(GLFW_KEY_R)) m_PlacementSystem->RotateGhost();
    if (m_Input.Pressed(GLFW_KEY_F)) HandleInteract(machineTarget, ray);
    if (m_Input.Pressed(GLFW_KEY_T) && hasMachine) InteractionSystem::TryRotateMachine(*m_Registry, machineTarget);
    if (m_Input.Pressed(GLFW_KEY_X) && hasMachine)
        InteractionSystem::TryPickupMachine(*m_Registry, machineTarget, m_PlayerEntity, m_World.placementGrid, m_Settings.terrain.cellSize);
    if (m_Input.MousePressed(GLFW_MOUSE_BUTTON_LEFT) && !mouseOverUI) m_InspectedEntity = machineTarget;

    // Camera
    if (m_Input.Pressed(GLFW_KEY_Q)) m_Camera->SnapRotateIso(false);
    if (m_Input.Pressed(GLFW_KEY_E)) m_Camera->SnapRotateIso(true);
    if (m_Input.Pressed(GLFW_KEY_TAB)) {
        if (rtsMode) {
            m_Camera->SetMode(Camera3D::Mode::Isometric); // isoTarget stays where it is — seamless transition
            ClearUnitSelection();
        } else {
            m_Camera->SetMode(Camera3D::Mode::IsoRTS);
        }
    }

    // Save / load
    if (m_Input.Pressed(GLFW_KEY_F5)) SaveCurrentGame();
    if (m_Input.Pressed(GLFW_KEY_F9)) m_PendingLoad = true; // same path as the Game Over "Load last save" button

    // Movement: RTS pans the camera, Isometric moves the player
    if (m_Camera->GetMode() == Camera3D::Mode::IsoRTS) {
        HandleRTSInput(ray);
        glm::vec3 panDir(0.0f);
        if (m_Input.Down(GLFW_KEY_W)) panDir.z -= 1.0f;
        if (m_Input.Down(GLFW_KEY_S)) panDir.z += 1.0f;
        if (m_Input.Down(GLFW_KEY_A)) panDir.x += 1.0f;
        if (m_Input.Down(GLFW_KEY_D)) panDir.x -= 1.0f;
        if (glm::length(panDir) > 0.001f)
            m_Camera->PanIso(glm::normalize(panDir), deltaTime, m_Input.Down(GLFW_KEY_LEFT_SHIFT));
    } else if (m_Camera->GetMode() == Camera3D::Mode::Isometric) {
        glm::vec3 moveDir(0.0f);
        if (m_Input.Down(GLFW_KEY_W)) moveDir.z += 1.0f;
        if (m_Input.Down(GLFW_KEY_S)) moveDir.z -= 1.0f;
        if (m_Input.Down(GLFW_KEY_A)) moveDir.x -= 1.0f;
        if (m_Input.Down(GLFW_KEY_D)) moveDir.x += 1.0f;
        if (m_Input.Down(GLFW_KEY_SPACE)) m_Camera->ProcessKeyboard(CameraMovement::Up, deltaTime);
        if (m_Input.Down(GLFW_KEY_LEFT_CONTROL)) m_Camera->ProcessKeyboard(CameraMovement::Down, deltaTime);
        if (glm::length(moveDir) > 0.0f && m_Registry->valid(m_PlayerEntity)) {
            m_Registry->remove<MoveOrderComponent>(m_PlayerEntity); // manual input overrides any queued order
            MovePlayer(glm::normalize(moveDir), deltaTime);
        }
    }

    float scrollDelta = m_VulkanEngine->GetScrollDelta();
    if (scrollDelta != 0.0f && !mouseOverUI)
        m_Camera->ProcessIsoZoom(scrollDelta * m_Settings.isoZoomSpeed);
    m_VulkanEngine->ResetScrollDelta();

    HandleHotbarInput();
    m_Camera->UpdateIso(deltaTime);
}

void Game::HandleInteract(entt::entity machineTarget, const MouseRay &ray) {
    auto &playerTransform = m_Registry->get<TransformComponent>(m_PlayerEntity);

    // No machine under the cursor: mine trees/ore instead
    if (!m_Registry->valid(machineTarget)) {
        InteractionSystem::TryMineAtCursor(*m_Registry, m_World, m_PlayerEntity, ray.origin, ray.dir, m_Settings.terrain,
                                           INTERACT_RANGE, m_VulkanEngine.get(), m_RenderSystem->GetMeshRenderer(),
                                           m_ChunkManager, *m_RenderSystem->GetTreeRenderer());
        return;
    }

    auto &machineTransform = m_Registry->get<TransformComponent>(machineTarget);
    if (glm::length(machineTransform.Position - playerTransform.Position) > INTERACT_RANGE) return;

    // Collect output first; if nothing to collect, try to fuel, then insert the selected item
    bool collected = m_Registry->any_of<MinerComponent>(machineTarget)
                         ? InteractionSystem::TryCollectMinerOutput(*m_Registry, machineTarget, m_PlayerEntity)
                         : InteractionSystem::TryCollectFromMachine(*m_Registry, machineTarget, m_PlayerEntity);
    if (collected || m_SelectedItem == ItemId::None) return;

    if (!InteractionSystem::TryFuelMachine(*m_Registry, machineTarget, m_PlayerEntity, m_SelectedItem, 1))
        InteractionSystem::TryInsertIntoMachine(*m_Registry, machineTarget, m_PlayerEntity, m_SelectedItem, 1);
}

void Game::HandleHotbarInput() {
    for (int i = 0; i < HOTBAR_SIZE; i++) {
        if (m_Input.Pressed(GLFW_KEY_1 + i)) {
            ItemId hotbarItem = m_Hotbar[i];
            m_SelectedItem = (m_SelectedItem == hotbarItem) ? ItemId::None : hotbarItem;
        }
    }
}

// Developer shortcuts. K = damage target under cursor (cycles damage types), F6/F7 = spawn enemy/friendly soldier.
void Game::HandleDebugInput(const MouseRay &ray) {
    if (m_Input.Pressed(GLFW_KEY_K) && !ImGui::GetIO().WantCaptureMouse) {
        entt::entity target = InteractionSystem::FindDamageableAlongRay(*m_Registry, ray.origin, ray.dir, UNIT_PICK_RANGE);
        if (m_Registry->valid(target)) {
            DamageType type = static_cast<DamageType>(m_DebugDamageType);
            auto &hp = m_Registry->get<HealthComponent>(target);
            float before = hp.current;
            DamageSystem::ApplyDamage(*m_Registry, target, 20.0f, type);
            std::cout << "[debug] damage type " << m_DebugDamageType << " dealt " << before - hp.current
                      << ", hp now " << hp.current << std::endl;
            m_DebugDamageType = (m_DebugDamageType + 1) % DAMAGE_TYPE_COUNT;
        }
    }

    bool spawnEnemy = m_Input.Pressed(GLFW_KEY_F6);
    bool spawnFriendly = m_Input.Pressed(GLFW_KEY_F7);
    if (spawnEnemy || spawnFriendly) {
        glm::vec3 ground = TerrainRaycast::RaycastToTerrain(ray.origin, ray.dir, m_Settings.terrain);
        SpawnUnit(ItemId::Soldier, ground, spawnEnemy ? 1 : PLAYER_TEAM);
    }
}

void Game::ClearUnitSelection() {
    auto selectables = m_Registry->view<SelectableComponent>();
    for (auto e : selectables) selectables.get<SelectableComponent>(e).isSelected = false;
    m_SelectedUnits.clear();
    m_IsBoxSelecting = false;
    m_RenderSystem->SetSelectionRect(false, {}, {});
}

void Game::IssueMoveCommand(const glm::vec3 &ground) {
    // Arrange selected units in a square grid around the target
    int n = static_cast<int>(m_SelectedUnits.size());
    int side = static_cast<int>(std::ceil(std::sqrt((float)std::max(n, 1))));
    int i = 0;
    for (auto unit : m_SelectedUnits) {
        if (!m_Registry->valid(unit)) continue;
        m_Registry->remove<AttackOrderComponent>(unit);
        glm::vec3 offset((i % side - (side - 1) * 0.5f) * 2.0f, 0.0f, (i / side - (side - 1) * 0.5f) * 2.0f);
        m_Registry->emplace_or_replace<MoveOrderComponent>(unit, MoveOrderComponent{ ground + offset });
        i++;
    }
}

void Game::MovePlayer(glm::vec3 direction, float deltaTime) {
    auto &transform = m_Registry->get<TransformComponent>(m_PlayerEntity);
    auto &player = m_Registry->get<PlayerComponent>(m_PlayerEntity);
    player.moveSpeed = m_Settings.playerMoveSpeed; // re-read each frame so settings changes apply live
    player.runSpeed = m_Settings.playerRunSpeed;

    // Move relative to the camera's facing, so "forward" is always "up the screen" on any 45° snap
    glm::vec3 forward = m_Camera->GetForwardDirection();
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
    float speed = m_Input.Down(GLFW_KEY_LEFT_SHIFT) ? player.runSpeed : player.moveSpeed;
    transform.Position += (forward * direction.z + right * direction.x) * speed * deltaTime;

    transform.Position.y = GpuNoiseMatch::SampleSurfaceHeight(transform.Position.x, transform.Position.z,
                                                              (float)m_Settings.terrain.seed);
}

void Game::HandleRTSInput(const MouseRay &ray) {
    ImGuiIO &io = ImGui::GetIO();
    const glm::vec2 mouseUI(io.MousePos.x, io.MousePos.y);
    const bool leftDown = m_Input.MouseDown(GLFW_MOUSE_BUTTON_LEFT);

    // ---- Left mouse: press starts a drag, release finalizes ----
    // Don't let clicks on UI windows start a selection
    if (m_Input.MousePressed(GLFW_MOUSE_BUTTON_LEFT) && !io.WantCaptureMouse) {
        m_IsBoxSelecting = true;
        m_BoxSelectStart = ray.cursor;
        m_BoxSelectStartUI = mouseUI;
    }

    if (m_Input.MouseReleased(GLFW_MOUSE_BUTTON_LEFT) && m_IsBoxSelecting) {
        const glm::vec2 boxEnd = ray.cursor;
        auto selectables = m_Registry->view<TransformComponent, SelectableComponent>();

        if (!m_Input.Down(GLFW_KEY_LEFT_SHIFT)) { // shift = add to selection
            for (auto e : selectables) selectables.get<SelectableComponent>(e).isSelected = false;
            m_SelectedUnits.clear();
        }

        auto addToSelection = [&](entt::entity e) {
            auto &sel = m_Registry->get<SelectableComponent>(e);
            if (sel.isSelected) return;
            sel.isSelected = true;
            m_SelectedUnits.push_back(e);
        };

        if (glm::length(boxEnd - m_BoxSelectStart) < CLICK_DRAG_THRESHOLD) {
            // Single click: ray-pick the nearest selectable
            entt::entity clicked = InteractionSystem::FindSelectableAlongRay(*m_Registry, ray.origin, ray.dir, UNIT_PICK_RANGE);
            if (m_Registry->valid(clicked)) addToSelection(clicked);
        } else {
            // Box select: project each unit to screen space
            glm::vec2 boxMin = glm::min(m_BoxSelectStart, boxEnd);
            glm::vec2 boxMax = glm::max(m_BoxSelectStart, boxEnd);
            glm::mat4 viewProj = m_Camera->GetViewProjection(ray.aspect);

            for (auto e : selectables) {
                glm::vec3 worldPos = selectables.get<TransformComponent>(e).Position + glm::vec3(0, 1.0f, 0);
                auto screen = Camera3D::ProjectToScreen(viewProj, worldPos, ray.screen);
                if (screen && glm::all(glm::greaterThanEqual(*screen, boxMin)) && glm::all(glm::lessThanEqual(*screen, boxMax)))
                    addToSelection(e);
            }
        }
        m_IsBoxSelecting = false;
    }

    // ---- Right mouse: attack the enemy under the cursor, or move to the ground point ----
    if (m_Input.MousePressed(GLFW_MOUSE_BUTTON_RIGHT) && !io.WantCaptureMouse) {
        entt::entity enemy = InteractionSystem::FindEnemyAlongRay(*m_Registry, ray.origin, ray.dir, UNIT_PICK_RANGE, PLAYER_TEAM);
        if (m_Registry->valid(enemy)) {
            for (auto unit : m_SelectedUnits) {
                if (!m_Registry->valid(unit) || !m_Registry->all_of<CombatStatsComponent>(unit)) continue;
                m_Registry->remove<MoveOrderComponent>(unit);
                m_Registry->emplace_or_replace<AttackOrderComponent>(unit, AttackOrderComponent{ enemy });
            }
        } else {
            IssueMoveCommand(TerrainRaycast::RaycastToTerrain(ray.origin, ray.dir, m_Settings.terrain));
        }
    }

    // ---- H: toggle hold fire on selected units ----
    if (m_Input.Pressed(GLFW_KEY_H)) {
        for (auto u : m_SelectedUnits) {
            if (!m_Registry->valid(u)) continue;
            auto &s = m_Registry->get_or_emplace<StanceComponent>(u);
            s.stance = (s.stance == Stance::FireAtWill) ? Stance::HoldFire : Stance::FireAtWill;
        }
    }

    // ---- Drag rectangle overlay ----
    bool showRect = m_IsBoxSelecting && leftDown && glm::length(mouseUI - m_BoxSelectStartUI) > CLICK_DRAG_THRESHOLD;
    m_RenderSystem->SetSelectionRect(showRect, m_BoxSelectStartUI, mouseUI);
}

void Game::HandleFreeFlyInput(float deltaTime) {
    bool sprint = m_Input.Down(GLFW_KEY_LEFT_SHIFT) || m_Input.Down(GLFW_KEY_RIGHT_SHIFT);

    if (m_Input.Down(GLFW_KEY_W)) m_Camera->ProcessKeyboard(CameraMovement::Forward, deltaTime, sprint);
    if (m_Input.Down(GLFW_KEY_S)) m_Camera->ProcessKeyboard(CameraMovement::Backward, deltaTime, sprint);
    if (m_Input.Down(GLFW_KEY_A)) m_Camera->ProcessKeyboard(CameraMovement::Left, deltaTime, sprint);
    if (m_Input.Down(GLFW_KEY_D)) m_Camera->ProcessKeyboard(CameraMovement::Right, deltaTime, sprint);
    if (m_Input.Down(GLFW_KEY_SPACE)) m_Camera->ProcessKeyboard(CameraMovement::Up, deltaTime, sprint);
    if (m_Input.Down(GLFW_KEY_LEFT_CONTROL)) m_Camera->ProcessKeyboard(CameraMovement::Down, deltaTime, sprint);
    if (m_Input.Down(GLFW_KEY_LEFT_SHIFT)) m_Camera->ProcessKeyboard(CameraMovement::Shift, deltaTime, sprint);

    double xpos, ypos;
    glfwGetCursorPos(m_VulkanEngine->GetWindow(), &xpos, &ypos);
    if (m_FirstMouse) {
        m_LastMouseX = xpos;
        m_LastMouseY = ypos;
        m_FirstMouse = false;
    }
    float deltaX = static_cast<float>(xpos - m_LastMouseX);
    float deltaY = static_cast<float>(m_LastMouseY - ypos);
    m_LastMouseX = xpos;
    m_LastMouseY = ypos;

    if (!ImGui::GetIO().WantCaptureMouse) m_Camera->ProcessMouseMovement(deltaX, deltaY);
}

// ============================================================================
// Simulation
// ============================================================================

void Game::Update(float deltaTime) {
    if (m_GameOver) return;

    m_TotalTime += deltaTime;
    m_RenderSystem->GetFogTexture()->SetTime(m_TotalTime);

    const bool hasPlayer = m_Registry->valid(m_PlayerEntity);
    const bool rtsMode = m_Camera->GetMode() == Camera3D::Mode::IsoRTS;

    if (hasPlayer) {
        auto &transform = m_Registry->get<TransformComponent>(m_PlayerEntity);
        if (m_Camera->GetMode() == Camera3D::Mode::Isometric) m_Camera->SetIsoTarget(transform.Position);

        entt::entity nearbyPickup = InteractionSystem::FindNearestPickup(*m_Registry, transform.Position, PICKUP_RADIUS);
        if (m_Registry->valid(nearbyPickup))
            InteractionSystem::CollectPickup(*m_Registry, nearbyPickup, m_PlayerEntity, m_AudioEvents.get());
    }

    const MouseRay ray = GetMouseRay();
    m_PlacementSystem->Update(*m_Registry, m_PlayerEntity, *m_Camera, m_SelectedItem, m_Settings.terrain, ray.cursor.x,
                              ray.cursor.y, ray.screen.x, ray.screen.y, ray.aspect, m_World.placementGrid,
                              m_VulkanEngine.get(), m_RenderSystem->GetMeshRenderer());

    m_DayNightCycle.Update(deltaTime);

    // Factory
    MinerSystem::Update(*m_Registry, m_World.depletionMap, m_Settings.terrain, deltaTime, m_ChunkManager);
    FurnaceSystem::Update(*m_Registry, deltaTime);

    int unitCount = 0;
    for ([[maybe_unused]] auto e : m_Registry->view<CombatStatsComponent, TeamComponent>()) unitCount++;
    std::vector<AssemblerSystem::SpawnRequest> spawns;
    AssemblerSystem::Update(*m_Registry, deltaTime, spawns, UNIT_CAP - unitCount);
    for (auto &s : spawns) SpawnUnit(s.unit, s.position, s.team);

    BeltSystem::Update(*m_Registry, m_World.placementGrid, m_Settings.terrain.cellSize, deltaTime);
    BeltRenderSystem::SyncItemEntities(*m_Registry, m_VulkanEngine.get(), m_RenderSystem->GetMeshRenderer(), m_Settings.terrain.cellSize);
    InserterSystem::Update(*m_Registry, m_World.placementGrid, m_Settings.terrain.cellSize, deltaTime);
    PowerSystem::Update(*m_Registry, deltaTime);

    // Units / combat
    MoveOrderSystem::Update(*m_Registry, deltaTime, m_Settings.terrain);
    DamageSystem::ProcessDeaths(*m_Registry, glm::vec3(0.0f), m_World.placementGrid, m_Settings.terrain.cellSize); // temporary respawn point
    if (VictorySystem::Update(*m_Registry, m_HadCatalyst) == GameResult::Lost) m_GameOver = true;

    m_Nests.Update(*m_Registry, m_Settings.terrain.seed, m_World.fog, deltaTime, m_TotalTime);

    std::vector<CombatSystem::Shot> shots;
    CombatSystem::Update(*m_Registry, deltaTime, shots);
    for (auto &s : shots) m_RenderSystem->AddTracer(s.from, s.to);

    // Minimap + chunk streaming follow the camera in RTS mode, the player otherwise
    glm::vec3 focus = (rtsMode || !hasPlayer) ? m_Camera->GetIsoTarget()
                                              : m_Registry->get<TransformComponent>(m_PlayerEntity).Position;
    m_HUD.SetMinimapFocus(glm::vec2(focus.x, focus.z));
    glm::vec3 camForward = m_Camera->GetForwardDirection(); // map turns with the camera: screen-up = map-up
    m_HUD.SetMinimapForward(glm::vec2(camForward.x, camForward.z));

    m_FogTimer += deltaTime;
    if (m_FogTimer >= FOG_UPDATE_INTERVAL) {
        m_FogTimer = 0.0f;
        m_World.fog.Update(*m_Registry, PLAYER_TEAM);
        glm::vec3 c = m_Camera->GetIsoTarget();
        FogTexture *fogTex = m_RenderSystem->GetFogTexture();
        if (m_World.fog.ConsumeDirty() || fogTex->WindowNeedsMove(c)) fogTex->Update(m_World.fog, c);
    }

    if (hasPlayer) UpdateChunkStreaming(focus);

    AnimationSystem::Update(*m_Registry, deltaTime);
}

FrameContext Game::MakeFrameContext() {
    return FrameContext{ *m_Registry,     *m_Camera,       m_Settings,     *m_AudioEngine,
                         m_World,         m_DayNightCycle, m_ChunkManager, m_PlayerEntity,
                         m_InspectedEntity, m_SelectedItem, m_ShowOptionsInPause };
}

void Game::Render() {
    m_RenderSystem->RenderFrame(MakeFrameContext(), false);
}

// Called by RenderSystem while the ImGui frame is open
void Game::DrawHUD() {
    m_HUD.DrawHealthBars(*m_Registry, *m_Camera, GetAspect());

    if (auto target = m_HUD.DrawMinimap(*m_Registry, *m_Camera, m_World.fog, m_Nests, !m_SelectedUnits.empty())) {
        float h = GpuNoiseMatch::SampleSurfaceHeight(target->x, target->y, (float)m_Settings.terrain.seed);
        IssueMoveCommand(glm::vec3(target->x, h, target->y));
    }

    if (m_GameOver) {
        switch (m_HUD.DrawGameOver()) {
        case GameHUD::GameOverChoice::LoadLastSave: m_PendingLoad = true; break;
        case GameHUD::GameOverChoice::MainMenu: m_PendingMenu = true; break;
        default: break;
        }
    }
}

// ============================================================================
// World setup, saving and loading
// ============================================================================

void Game::CreateInitialEntities() {
    std::mt19937 rng(m_Settings.terrain.seed);
    TransformComponent spawn;
    spawn.Position = WorldGenerator::FindSpawnPoint(m_Settings.terrain, m_Settings.terrain.seed, rng);
    m_PlayerEntity = EntityFactory::CreatePlayer(*m_Registry, spawn, m_UnitMesh);
}

entt::entity Game::SpawnUnit(ItemId unitItem, glm::vec3 position, uint8_t team) {
    return EntityFactory::CreateUnit(*m_Registry, unitItem, position, team, m_UnitMesh, m_Settings.terrain.seed);
}

void Game::ResetWorldState() {
    m_ChunkManager.UnloadAll(m_RenderSystem->GetTerrainRenderer(), m_RenderSystem->GetTreeRenderer());

    m_Registry->clear();
    m_World = WorldState{};
    m_Nests.Clear();

    m_PlayerEntity = entt::null;
    m_InspectedEntity = entt::null;
    m_SelectedUnits.clear();
    m_IsBoxSelecting = false;
    m_HadCatalyst = false;
    m_GameOver = false;
}

void Game::StartNewGame(const std::string &saveName) {
    ResetWorldState();
    m_ChunkManager.Reset();
    CreateInitialEntities();

    m_CurrentSaveName = saveName;
    SaveCurrentGame(); // save immediately so the file exists
    BeginChunkLoad();
}

void Game::LoadExistingGame(const std::string &saveName) {
    m_ChunkManager.Reset();
    if (!LoadSave(saveName)) {
        std::cerr << "Failed to load save: " << saveName << std::endl;
        return; // stay in MainMenu
    }
    m_CurrentSaveName = saveName;
    BeginChunkLoad();
}

// Clears the world and loads Saves/<saveName>.json, including saved units.
bool Game::LoadSave(const std::string &saveName) {
    ResetWorldState();
    std::vector<SavedUnit> savedUnits;
    entt::entity loadedPlayer = entt::null;
    bool ok = SaveManager::LoadGame("Saves/" + saveName + ".json", *m_Registry, loadedPlayer, m_World, m_Settings,
                                    m_VulkanEngine.get(), m_RenderSystem->GetMeshRenderer(), m_UnitMesh, savedUnits);
    if (!ok) return false;

    m_PlayerEntity = loadedPlayer;
    for (auto &su : savedUnits) {
        entt::entity e = SpawnUnit(su.item, su.pos, su.team);
        if (m_Registry->valid(e)) m_Registry->get<HealthComponent>(e).current = su.health;
    }
    return true;
}

void Game::SaveCurrentGame() {
    SaveManager::SaveGame("Saves/" + m_CurrentSaveName + ".json", *m_Registry, m_PlayerEntity, m_World, m_Settings);
}

// Kicks off the initial chunk load around the player and switches to the loading screen.
void Game::BeginChunkLoad() {
    m_ChunkManager.BeginInitialLoad(m_Registry->get<TransformComponent>(m_PlayerEntity).Position,
                                    m_Camera->GetForwardDirection());
    m_State = GameState::Loading;
}

void Game::UpdateChunkStreaming(const glm::vec3 &center) {
    m_ChunkManager.Update(center, m_Camera->GetForwardDirection(), m_Camera->GetIsoDistance(),
                          m_RenderSystem->GetTerrainRenderer(), m_RenderSystem->GetTreeRenderer());
}

void Game::RefreshSaveList() {
    m_AvailableSaves.clear();
    if (!std::filesystem::exists("Saves")) {
        std::filesystem::create_directories("Saves");
        return;
    }
    for (auto &entry : std::filesystem::directory_iterator("Saves")) {
        if (entry.path().extension() == ".json") m_AvailableSaves.push_back(entry.path().stem().string());
    }
}

// ============================================================================
// Screens
// ============================================================================

void Game::RunMainMenu() {
    if (!m_VulkanEngine->BeginFrame()) return; // swapchain recreation in progress — skip this frame
    m_RenderSystem->GetImGuiUtil()->NewFrame();

    ImGuiIO &io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 350));
    ImGui::Begin("Main Menu", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

    ImGui::Text("My Factory Game");
    ImGui::Separator();

    ImGui::InputText("World Name", m_NewGameNameBuffer, sizeof(m_NewGameNameBuffer));
    if (ImGui::Button("New Game", ImVec2(-1, 40))) StartNewGame(m_NewGameNameBuffer);

    ImGui::Separator();
    ImGui::Text("Load Game:");
    RefreshSaveList();
    for (auto &save : m_AvailableSaves) {
        if (ImGui::Button(save.c_str(), ImVec2(-1, 0))) LoadExistingGame(save);
    }

    ImGui::Separator();
    ImGui::BeginDisabled(true);
    ImGui::Button("Multiplayer (coming soon)", ImVec2(-1, 40));
    ImGui::EndDisabled();

    if (ImGui::Button("Options", ImVec2(-1, 40))) m_ShowOptionsInMenu = true;
    if (ImGui::Button("Quit", ImVec2(-1, 40))) m_IsRunning = false;

    ImGui::End();

    if (m_ShowOptionsInMenu) {
        ImGui::Begin("Options", &m_ShowOptionsInMenu);
        // TODO: reuse the settings sliders from DebugUI's settings tab here
        ImGui::End();
    }

    m_RenderSystem->GetImGuiUtil()->RenderDrawData(m_VulkanEngine->GetCurrentCommandBuffer());
    m_VulkanEngine->EndFrame();
}

void Game::RunPauseMenu() {
    PauseMenuAction action = m_RenderSystem->RenderFrame(MakeFrameContext(), true);

    switch (action) {
    case PauseMenuAction::Resume:
        m_State = GameState::Playing;
        break;
    case PauseMenuAction::SaveGame:
        SaveCurrentGame();
        break;
    case PauseMenuAction::QuitToMenu:
        ResetWorldState();
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
    if (m_Registry->valid(m_PlayerEntity))
        UpdateChunkStreaming(m_Registry->get<TransformComponent>(m_PlayerEntity).Position);

    bool frameStarted = m_VulkanEngine->BeginFrame([&](VkCommandBuffer cmd) {
        m_ChunkManager.RecordPendingGeneration(cmd, m_RenderSystem->GetChunkGenerator(), m_RenderSystem->GetTerrainRenderer(),
                                               m_RenderSystem->GetTreeRenderer(), m_Settings.terrain, m_World.depletionMap,
                                               m_World.removedTrees);
    });
    if (!frameStarted) return;

    m_RenderSystem->GetImGuiUtil()->NewFrame();

    ImGuiIO &io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 100));
    ImGui::Begin("Loading", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
    ImGui::Text("Generating world...");
    ImGui::ProgressBar(m_ChunkManager.GetInitialLoadProgress(), ImVec2(-1, 30));
    ImGui::End();

    m_RenderSystem->GetImGuiUtil()->RenderDrawData(m_VulkanEngine->GetCurrentCommandBuffer());
    m_VulkanEngine->EndFrame();
}
