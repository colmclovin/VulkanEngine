#pragma once
#include <memory>
#include <entt/entt.hpp>
class VulkanEngine;
class QuadRenderer;

//class ResourceManager;
//class MeshRenderer;
class Camera3D;

class RenderSystem {

public:
    RenderSystem(VulkanEngine *engine);
    ~RenderSystem();
    
    
    void Init();
    void RenderFrame(entt::registry &registry);//, const Camera3D &camera);
    void Shutdown();

    //ResourceManager *GetResourceManager() const { return m_ResourceManager.get(); }
    //MeshRenderer *GetMeshRenderer() const { return m_MeshRenderer.get(); }
    QuadRenderer *GetQuadRenderer() const { return m_QuadRenderer.get(); }
    
private: 
    VulkanEngine *m_Engine = nullptr;

    // Rendering subsystems
    //std::unique_ptr<ResourceManager> m_ResourceManager;
    //std::unique_ptr<MeshRenderer> m_MeshRenderer;
    std::unique_ptr<QuadRenderer> m_QuadRenderer;
    bool m_initialized = false;
};