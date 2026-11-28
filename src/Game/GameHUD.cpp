#include "GameHUD.h"
#include <algorithm>
#include <imgui/imgui.h>
#include "../Components/Components.h"
#include "Camera3D.h"
#include "EntityFactory.h"
#include "FogOfWar.h"
#include "NestManager.h"

void GameHUD::DrawHealthBars(entt::registry &registry, const Camera3D &camera, float aspect) {
    ImGuiIO &io = ImGui::GetIO();
    const glm::vec2 displaySize(io.DisplaySize.x, io.DisplaySize.y);
    const glm::mat4 viewProj = camera.GetViewProjection(aspect);
    auto *dl = ImGui::GetBackgroundDrawList();

    auto view = registry.view<TransformComponent, HealthComponent>();
    for (auto e : view) {
        auto &hp = view.get<HealthComponent>(e);
        if (hp.current >= hp.max) continue; // only show damaged things

        auto screen = Camera3D::ProjectToScreen(viewProj, view.get<TransformComponent>(e).Position + glm::vec3(0, 3.0f, 0), displaySize);
        if (!screen) continue;
        ImVec2 s(screen->x, screen->y);
        float frac = glm::clamp(hp.current / hp.max, 0.0f, 1.0f);
        dl->AddRectFilled(ImVec2(s.x - 20, s.y), ImVec2(s.x + 20, s.y + 4), IM_COL32(40, 40, 40, 200));
        dl->AddRectFilled(ImVec2(s.x - 20, s.y), ImVec2(s.x - 20 + 40 * frac, s.y + 4), IM_COL32(220, 60, 60, 255));
    }
}

std::optional<glm::vec2> GameHUD::DrawMinimap(entt::registry &registry, Camera3D &camera, const FogOfWar &fog,
                                              const NestManager &nests, bool hasSelection) {
    ImGuiIO &io = ImGui::GetIO();
    const float size = MINIMAP_SIZE_PX;

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 10.0f, io.DisplaySize.y - 10.0f), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
    ImGui::SetNextWindowSize(ImVec2(size, size));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Minimap", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);

    ImGui::Image((ImTextureID)(uintptr_t)m_Minimap.GetImGuiTexture(), ImVec2(size, size));
    const ImVec2 imgMin = ImGui::GetItemRectMin();
    const bool hovered = ImGui::IsItemHovered();
    const glm::vec2 center = GetMinimapCenter();

    auto worldToScreen = [&](float wx, float wz) {
        float u = (wx - center.x) / (m_MinimapRange * 2.0f) + 0.5f;
        float v = (wz - center.y) / (m_MinimapRange * 2.0f) + 0.5f;
        return ImVec2(imgMin.x + u * size, imgMin.y + v * size);
    };
    auto outsideMap = [&](ImVec2 s, float padX = 0.0f, float padZ = 0.0f) {
        return s.x + padX < imgMin.x || s.y + padZ < imgMin.y || s.x - padX > imgMin.x + size || s.y - padZ > imgMin.y + size;
    };

    ImDrawList *dl = ImGui::GetWindowDrawList();
    dl->AddRect(imgMin, ImVec2(imgMin.x + size, imgMin.y + size), IM_COL32(200, 200, 200, 255));

    // Unit dots: own team always, other teams only where currently visible
    auto units = registry.view<TransformComponent, TeamComponent>();
    for (auto e : units) {
        glm::vec3 p = units.get<TransformComponent>(e).Position;
        bool friendly = units.get<TeamComponent>(e).team == EntityFactory::PLAYER_TEAM;
        if (!friendly && fog.GetState(p.x, p.z) != FogOfWar::Visible) continue;
        ImVec2 s = worldToScreen(p.x, p.z);
        if (outsideMap(s)) continue;
        dl->AddCircleFilled(s, 3.0f, friendly ? IM_COL32(80, 255, 120, 255) : IM_COL32(255, 70, 70, 255));
    }

    // Buildings (anything with bounds that isn't a unit or a pickup), once explored
    auto placed = registry.view<TransformComponent, BoundsComponent>(entt::exclude<SelectableComponent, PickupComponent>);
    float pxPerUnit = size / (m_MinimapRange * 2.0f);
    for (auto e : placed) {
        glm::vec3 p = placed.get<TransformComponent>(e).Position;
        if (fog.GetState(p.x, p.z) == FogOfWar::Unexplored) continue;
        glm::vec3 he = placed.get<BoundsComponent>(e).halfExtents;
        ImVec2 s = worldToScreen(p.x, p.z);
        float hx = std::max(he.x * pxPerUnit, 1.5f);
        float hz = std::max(he.z * pxPerUnit, 1.5f);
        if (outsideMap(s, hx, hz)) continue;
        dl->AddRectFilled(ImVec2(s.x - hx, s.y - hz), ImVec2(s.x + hx, s.y + hz), IM_COL32(240, 170, 60, 255));
    }

    // Active enemy nests the player can currently see. Tier 0 = pale red, tier 4 = deep red; bigger dot for higher tier
    for (const auto &nest : nests.Nests()) {
        if (fog.GetState(nest.pos.x, nest.pos.y) != FogOfWar::Visible) continue;
        ImVec2 s = worldToScreen(nest.pos.x, nest.pos.y);
        if (outsideMap(s)) continue;
        int t = nest.tier;
        dl->AddCircleFilled(s, 3.0f + t, IM_COL32(255, 200 - t * 45, 200 - t * 45, 255));
    }

    // Camera marker
    glm::vec3 cam = camera.GetIsoTarget();
    ImVec2 cs = worldToScreen(cam.x, cam.z);
    dl->AddRect(ImVec2(cs.x - 5, cs.y - 5), ImVec2(cs.x + 5, cs.y + 5), IM_COL32(255, 255, 255, 255));

    // Input: left-drag pans the camera, wheel zooms, right-click issues a move order
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        m_MinimapDragCenter = m_MinimapCenter; // lock before anything moves
        m_MinimapDragging = true;
    }
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) m_MinimapDragging = false;

    if (hovered && io.MouseWheel != 0.0f)
        m_MinimapRange = glm::clamp(m_MinimapRange * (io.MouseWheel > 0 ? 0.85f : 1.18f), MINIMAP_MIN_RANGE, MINIMAP_MAX_RANGE);

    // Clamp so dragging past the edge keeps the target on the map
    float u = glm::clamp((io.MousePos.x - imgMin.x) / size, 0.0f, 1.0f);
    float v = glm::clamp((io.MousePos.y - imgMin.y) / size, 0.0f, 1.0f);
    glm::vec2 world = GetMinimapCenter() + glm::vec2(u - 0.5f, v - 0.5f) * (m_MinimapRange * 2.0f);

    if (m_MinimapDragging) {
        if (camera.GetMode() != Camera3D::Mode::IsoRTS) camera.SetMode(Camera3D::Mode::IsoRTS);
        camera.SetIsoTarget(glm::vec3(world.x, 0.0f, world.y));
    }

    std::optional<glm::vec2> moveOrder;
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && hasSelection) moveOrder = world;

    ImGui::End();
    ImGui::PopStyleVar();
    return moveOrder;
}

GameHUD::GameOverChoice GameHUD::DrawGameOver() {
    GameOverChoice choice = GameOverChoice::None;
    ImGuiIO &io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::Begin("Game Over", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
    ImGui::Text("Your Catalyst was destroyed.");
    if (ImGui::Button("Load last save")) choice = GameOverChoice::LoadLastSave;
    ImGui::SameLine();
    if (ImGui::Button("Main menu")) choice = GameOverChoice::MainMenu;
    ImGui::End();
    return choice;
}
