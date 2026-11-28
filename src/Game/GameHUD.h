#pragma once
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <optional>
#include <vulkan/vulkan.h>
#include "../Rendering/Minimap.h"

class Camera3D;
class FogOfWar;
class NestManager;
class VulkanEngine;
class FogTexture;

// In-game ImGui overlays: world-space health bars, the minimap, and the game-over panel.
class GameHUD {
public:
    enum class GameOverChoice { None, LoadLastSave, MainMenu };

    void Init(VulkanEngine *engine, FogTexture *fogTexture) { m_Minimap.Init(engine, fogTexture); }
    // Renders the minimap texture; call from the pre-render callback (before the main pass)
    void RecordMinimap(VkCommandBuffer cmd, int seed) { m_Minimap.Record(cmd, GetMinimapCenter(), m_MinimapRange, (float)seed); }

    // Where the minimap is centered when the player isn't dragging it (the player, or the RTS camera)
    void SetMinimapFocus(glm::vec2 worldXZ) { m_MinimapCenter = worldXZ; }

    void DrawHealthBars(entt::registry &registry, const Camera3D &camera, float aspect);

    // Left-drag pans the camera (switching to RTS mode). Returns a world XZ point if the player
    // right-clicked the map to send selected units there.
    std::optional<glm::vec2> DrawMinimap(entt::registry &registry, Camera3D &camera, const FogOfWar &fog,
                                         const NestManager &nests, bool hasSelection);

    GameOverChoice DrawGameOver();

private:
    static constexpr float MINIMAP_SIZE_PX = 220.0f;
    static constexpr float MINIMAP_MIN_RANGE = 50.0f;
    static constexpr float MINIMAP_MAX_RANGE = 2000.0f;

    glm::vec2 GetMinimapCenter() const { return m_MinimapDragging ? m_MinimapDragCenter : m_MinimapCenter; }

    Minimap m_Minimap;
    float m_MinimapRange = 150.0f;   // half-width in world units
    glm::vec2 m_MinimapCenter{};
    glm::vec2 m_MinimapDragCenter{}; // center locked when a drag starts, so the map doesn't slide under the cursor
    bool m_MinimapDragging = false;
};
