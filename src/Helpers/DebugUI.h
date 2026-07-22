#pragma once

#include <entt/entt.hpp>

class RenderSystem;

class DebugUI {
public:
    void Draw(entt::registry& registry, RenderSystem* renderSystem);

private:
    void DrawStatsPanel();
    void DrawEntityInspector(entt::registry& registry);
    // add more panels as they come: DrawSceneHierarchy(), DrawMaterialEditor(), etc.

    bool m_ShowDemo = false;   // handy toggle to keep around for reference/debugging
};