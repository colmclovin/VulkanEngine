#pragma once
#include <entt/entt.hpp>
class VulkanEngine;
class RenderSystem;
class Camera3D;

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
    void Update(float deltaTime);
    void Render();
    void Shutdown();
    // Core systems
    std::unique_ptr<VulkanEngine> m_VulkanEngine;
    std::unique_ptr<RenderSystem> m_RenderSystem;
    //std::unique_ptr<Camera3D> m_Camera;
    std::unique_ptr<entt::registry> m_Registry;


    bool m_IsRunning = false;

};