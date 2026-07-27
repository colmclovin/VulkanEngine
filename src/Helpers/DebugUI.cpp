// DebugUI.cpp
#include "DebugUI.h"
#include <imgui.h>
#include "../Components/Components.h"
#include "../Game/Camera3D.h"
#include "../Components/GameSettings.h"

void DebugUI::Draw(entt::registry& registry, RenderSystem* renderSystem, Camera3D* camera, GameSettings& settings) {
    if (m_ShowDemo) {
        ImGui::ShowDemoWindow(&m_ShowDemo);
    }

    ImGui::Begin("Inspector");

    if (ImGui::BeginTabBar("InspectorTabs")) {

        if (ImGui::BeginTabItem("Overview")) {
            DrawStats();
            ImGui::Separator();
            DrawEntityList(registry);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Settings")) {
            DrawSettingsTab(camera, settings);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void DebugUI::DrawStats() {
    ImGui::Text("Frame time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
    ImGui::Checkbox("Show Demo Window", &m_ShowDemo);
}

void DebugUI::DrawEntityList(entt::registry& registry) {
    auto view = registry.view<SpriteComponent>();
    for (auto entity : view) {
        auto& sprite = view.get<SpriteComponent>(entity);
        ImGui::PushID(static_cast<int>(entity));
        if (ImGui::TreeNode("Entity")) {
            ImGui::DragFloat2("Position", &sprite.transform.Position.x);
            ImGui::DragFloat2("Size", &sprite.transform.Scale.x);
            ImGui::ColorEdit4("Color", &sprite.color.x);
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
}

void DebugUI::DrawSettingsTab(Camera3D* camera, GameSettings& settings) {
    if (ImGui::CollapsingHeader("Camera - Free Fly", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::SliderFloat("Move Speed##freefly", &settings.freeFlyMoveSpeed, 0.5f, 20.0f)) {
            camera->SetMovementSpeed(settings.freeFlyMoveSpeed);
        }
        if (ImGui::SliderFloat("Mouse Sensitivity", &settings.freeFlySensitivity, 0.01f, 1.0f)) {
            camera->SetMouseSensitivity(settings.freeFlySensitivity);
        }
    }

    if (ImGui::CollapsingHeader("Camera - Isometric", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::SliderFloat("Rotate Speed", &settings.isoRotateSpeed, 1.0f, 30.0f)) {
            camera->SetIsoRotateSpeed(settings.isoRotateSpeed);
        }
        if (ImGui::SliderFloat("Distance", &settings.isoDistance, 5.0f, 60.0f)) {
            camera->SetIsoDistance(settings.isoDistance);
        }
        if (ImGui::SliderFloat("Pitch", &settings.isoPitch, 10.0f, 80.0f)) {
            camera->SetIsoPitch(settings.isoPitch);
        }
        ImGui::SliderFloat("Zoom Speed", &settings.isoZoomSpeed, 0.1f, 10.0f);
           
        
    }

    if (ImGui::CollapsingHeader("Player", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Move Speed##player", &settings.playerMoveSpeed, 0.5f, 20.0f);
        // consumed directly by Game::MovePlayer each frame — see note below
    }

    if (ImGui::CollapsingHeader("Terrain Generation", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragInt("Grid Width", &settings.terrain.gridWidth, 1, 8, 512);
        ImGui::DragInt("Grid Depth", &settings.terrain.gridDepth, 1, 8, 512);
        ImGui::DragFloat("Cell Size", &settings.terrain.cellSize, 0.1f, 0.1f, 10.0f);
        ImGui::DragFloat("Height Scale", &settings.terrain.heightScale, 0.1f, 0.0f, 50.0f);
        ImGui::DragFloat("Noise Scale", &settings.terrain.noiseScale, 0.001f, 0.001f, 1.0f, "%.3f");
        ImGui::DragInt("Seed", &settings.terrain.seed, 1, 0, 99999);

        if (ImGui::Button("Regenerate Terrain")) {
            m_RegenerateTerrainRequested = true;
        }
    }
    if (ImGui::Button("Save Settings")) {
        settings.SaveToFile("settings.json");
    }
    ImGui::SameLine();
    if (ImGui::Button("Reload Settings")) {
        settings = GameSettings::LoadFromFile("settings.json");
    }

    if (ImGui::CollapsingHeader("Rendering")) {
        ImGui::Checkbox("Wireframe Mode", &settings.wireframeMode);
        ImGui::ColorEdit4("Clear Color", &settings.clearColor.x);
    }
}