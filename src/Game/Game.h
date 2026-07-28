#pragma once
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include "../Components/GameSettings.h"
#include "../Audio/AudioEngine.h"
#include "../Audio/AudioEventSystem.h"

class TerrainGenerator;
class VulkanEngine;
class RenderSystem;
class Camera3D;
class AudioEngine;
struct GLFWwindow;


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
    entt::entity m_PlayerEntity = entt::null;
    entt::entity m_TerrainEntity = entt::null;
    GameSettings m_Settings;
    bool m_FirstMouse = true;
    double m_LastMouseX = 0.0, m_LastMouseY = 0.0;


    bool m_IsRunning = false;
    bool m_Initialized = false;
};