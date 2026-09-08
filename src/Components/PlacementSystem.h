// PlacementSystem.h
#pragma once
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include "../Game/ItemDatabase.h"
#include "../Game/Camera3D.h"
#include "Gamesettings.h"
#include "../Components/ResourceMap.h"
#include "../Components/PlacementGrid.h"

class PlacementSystem {
public:
    void Update(entt::registry& registry, entt::entity player, Camera3D& camera,
        ItemId selectedItem, const TerrainSettings& terrainSettings,
                float mouseX, float mouseY, float screenWidth, float screenHeight, float aspect, PlacementGrid &placementGrid);
    void TryConfirmPlacement(entt::registry &registry, entt::entity player, ResourceMap &resourceMap, PlacementGrid &placementGrid);
    void CancelPlacement(entt::registry& registry);
    //glm::vec3 RaycastToTerrain(glm::vec3 rayOrigin, glm::vec3 rayDir, const TerrainSettings& terrainSettings);
    bool IsPlacing() const { return m_GhostEntity != entt::null; }
    glm::vec3 SnapToGrid(glm::vec3 pos, float gridSize);   // add this declaration if missing
    void RotateGhost();
    glm::vec3 GetFacingFromRotation() const;

private:
    entt::entity m_GhostEntity = entt::null;
    ItemId m_PendingItem = ItemId::None;
    int m_RotationSteps = 0; // 0-3, each step = 90 degrees
};