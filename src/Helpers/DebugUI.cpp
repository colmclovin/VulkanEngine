// DebugUI.cpp
#include "DebugUI.h"
#include <imgui.h>
#include "../Components/Components.h"
#include "../Game/Camera3D.h"
#include "../Components/GameSettings.h"
#include "../Audio/AudioEngine.h"
#include "../Game/CraftingSystem.h"
#include "../Game/RecipeDatabase.h"
#include <entt/entt.hpp>

void DebugUI::Draw(entt::registry &registry, RenderSystem *renderSystem, Camera3D *camera, GameSettings &settings, AudioEngine *audioEngine, entt::entity m_PlayerEntity, entt::entity m_InspectedEntity) {

    if (m_ShowDemo) {
        ImGui::ShowDemoWindow(&m_ShowDemo);
    }

    ImGui::Begin("Inspector");

    if (ImGui::BeginTabBar("InspectorTabs")) {

        if (ImGui::BeginTabItem("Overview")) {
            DrawStats(registry);
            ImGui::Separator();
            DrawEntityList(registry);
            DrawInventory(registry, m_PlayerEntity);
			DrawCrafting(registry, m_PlayerEntity);
            DrawMachineInspector(registry, m_InspectedEntity);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Settings")) {
            DrawSettingsTab(camera, settings, audioEngine);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void DebugUI::DrawStats(entt::registry &registry) {
    ImGui::Text("Frame time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
    ImGui::Text("Frame rate: %.3f fps", ImGui::GetIO().Framerate);
    ImGui::Text("Total entities: %zu", registry.storage<entt::entity>().size());
    ImGui::Checkbox("Show Demo Window", &m_ShowDemo);
}
void DebugUI::DrawInventory(entt::registry &registry, entt::entity player) {
    if (!registry.valid(player) || !registry.any_of<InventoryComponent>(player)) return;
    auto &inv = registry.get<InventoryComponent>(player);
    ImGui::Begin("Inventory");
    for (auto &slot : inv.slots) {
        if (slot.item != ItemId::None) {
            ImGui::Text("%s x%d", ItemDatabase::Get(slot.item).name.c_str(), slot.count);
        }
    }
    ImGui::End();
}
void DebugUI::DrawCrafting(entt::registry& registry, entt::entity player) {
    ImGui::Begin("Crafting");
    for (const auto& recipe : RecipeDatabase::GetAll()) {
        bool canCraft = CraftingSystem::CanCraft(registry, player, recipe);
        ImGui::BeginDisabled(!canCraft);
        if (ImGui::Button(recipe.name.c_str())) {
            CraftingSystem::TryCraft(registry, player, recipe);
        }
        ImGui::EndDisabled();
    }
    ImGui::End();
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

void DebugUI::DrawSettingsTab(Camera3D* camera, GameSettings& settings, AudioEngine* audioEngine) {
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
    if (ImGui::CollapsingHeader("Audio", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::SliderFloat("Master Volume", &settings.masterVolume, 0.0f, 1.0f)) {
            audioEngine->SetMasterVolume(settings.masterVolume);
        }
        if (ImGui::SliderFloat("Music Volume", &settings.musicVolume, 0.0f, 1.0f)) {
            audioEngine->SetMusicVolume(settings.musicVolume);
        }
        if (ImGui::SliderFloat("SFX Volume", &settings.sfxVolume, 0.0f, 1.0f)) {
            audioEngine->SetSFXVolume(settings.sfxVolume);
        }
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
        ImGui::Checkbox("Show Bounds Boxes", &settings.showBoundsDebug);
        ImGui::Checkbox("Show Placement Grid", &settings.showGridDebug);
        ImGui::Checkbox("Wireframe Mode", &settings.wireframeMode);
        ImGui::ColorEdit4("Clear Color", &settings.clearColor.x);
    }
}
void DebugUI::DrawMachineInspector(entt::registry &registry, entt::entity target) {
    if (!registry.valid(target)) return;

    ImGui::Begin("Machine Inspector");

    if (registry.any_of<MinerComponent>(target)) {
        auto &miner = registry.get<MinerComponent>(target);
        ImGui::Text("Type: Miner");
        ImGui::Text("Status: %s", (miner.fuelRemaining > 0.0f) ? "Running" : "Idle (no fuel)");
        ImGui::Text("Output item: %s", miner.outputItem != ItemId::None ? ItemDatabase::Get(miner.outputItem).name.c_str() : "None");
        ImGui::Text("Output buffer: %d / %d", miner.outputBuffer, miner.outputBufferCapacity);
        ImGui::Text("Fuel: %d loaded, %.1fs remaining", miner.fuelBuffer, miner.fuelRemaining);
    }

    if (registry.any_of<FurnaceComponent>(target)) {
        auto &furnace = registry.get<FurnaceComponent>(target);
        ImGui::Text("Type: Furnace");
        ImGui::Text("Status: %s", furnace.isCooking ? "Cooking" : "Idle");
        if (furnace.isCooking) ImGui::Text("Cook timer: %.1fs left", furnace.cookTimer);
        ImGui::Text("Fuel: %d loaded, %.1fs remaining", furnace.fuelBuffer, furnace.fuelRemaining);
    }

    if (registry.any_of<AssemblerComponent>(target)) {
        auto &assembler = registry.get<AssemblerComponent>(target);
        ImGui::Text("Type: Assembler");
        ImGui::Text("Status: %s", assembler.isCrafting ? "Crafting" : "Idle");
        if (assembler.selectedRecipeIndex >= 0) {
            auto &recipes = RecipeDatabase::GetAll();
            if (assembler.selectedRecipeIndex < (int)recipes.size()) {
                ImGui::Text("Recipe: %s", recipes[assembler.selectedRecipeIndex].name.c_str());
            }
        } else {
            ImGui::Text("Recipe: none selected");
        }
        ImGui::Separator();
        ImGui::Text("Select Recipe:");
        auto& recipes = RecipeDatabase::GetAll();
        for (int i = 0; i < (int)recipes.size(); i++) {
            ImGui::PushID(i);
            if (ImGui::Button(recipes[i].name.c_str())) {
                assembler.selectedRecipeIndex = i;
            }
            ImGui::PopID();
        }
    }

    if (registry.any_of<BeltComponent>(target)) {
        auto &belt = registry.get<BeltComponent>(target);
        ImGui::Text("Type: Belt");

        ImGui::Text("Left lane: %d/%d", (int)belt.leftLane.queue.size(), belt.leftLane.capacity);
        for (auto &item : belt.leftLane.queue) {
            if (item.item != ItemId::None) {
                ImGui::Text("  %s (%.0f%%)", ItemDatabase::Get(item.item).name.c_str(), item.progress * 100.0f);
            }
        }

        ImGui::Text("Right lane: %d/%d", (int)belt.rightLane.queue.size(), belt.rightLane.capacity);
        for (auto &item : belt.rightLane.queue) {
            if (item.item != ItemId::None) {
                ImGui::Text("  %s (%.0f%%)", ItemDatabase::Get(item.item).name.c_str(), item.progress * 100.0f);
            }
        }
    }
    if (registry.any_of<InserterComponent>(target)) {
        auto &inserter = registry.get<InserterComponent>(target);
        ImGui::Text("Type: Inserter");
        ImGui::Text("Holding: %s", inserter.holdingItem ? ItemDatabase::Get(inserter.heldItem).name.c_str() : "Nothing");
    }

    if (registry.any_of<MachineInventoryComponent>(target)) {
        auto &inv = registry.get<MachineInventoryComponent>(target);
        ImGui::Separator();
        ImGui::Text("Inputs:");
        for (auto &slot : inv.inputs) {
            if (slot.item != ItemId::None) ImGui::Text("  %s x%d / %d", ItemDatabase::Get(slot.item).name.c_str(), slot.count, slot.capacity);
        }
        ImGui::Text("Outputs:");
        for (auto &slot : inv.outputs) {
            if (slot.item != ItemId::None) ImGui::Text("  %s x%d / %d", ItemDatabase::Get(slot.item).name.c_str(), slot.count, slot.capacity);
        }
    }

    ImGui::End();
}