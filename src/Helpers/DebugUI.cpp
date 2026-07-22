// DebugUI.cpp
#include "DebugUI.h"
#include <imgui.h>
#include "../Components/Components.h"

void DebugUI::Draw(entt::registry& registry, RenderSystem* renderSystem) {
    if (m_ShowDemo) {
        ImGui::ShowDemoWindow(&m_ShowDemo);
    }

    ImGui::Begin("Inspector");

    DrawStatsPanel();
    ImGui::Separator();
    DrawEntityInspector(registry);

    ImGui::End();
}

void DebugUI::DrawStatsPanel() {
    //ImGui::Begin("Stats");
    ImGui::Text("Frame time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
    ImGui::Checkbox("Show Demo Window", &m_ShowDemo);
    //ImGui::End();
}

void DebugUI::DrawEntityInspector(entt::registry& registry) {
   // ImGui::Begin("Entities");
    auto view = registry.view<SpriteComponent>();
    for (auto entity : view) {
        auto& sprite = view.get<SpriteComponent>(entity);
        ImGui::PushID(static_cast<int>(entity));
        ImGui::DragFloat3("Position", &sprite.transform.Position.x);
        ImGui::DragFloat3("Size", &sprite.transform.Scale.x);
		ImGui::DragFloat3("Rotation", &sprite.transform.Rotation.x);
        ImGui::ColorEdit4("Color", &sprite.color.x);
        ImGui::PopID();
        ImGui::Separator();
    }
    //ImGui::End();
}