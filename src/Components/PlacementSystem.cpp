// PlacementSystem.cpp
#include "PlacementSystem.h"
#include "Components.h"
#include "../Game/PlaceableDatabase.h"
#include "../Components/ModelLoader.h"
#include "TerrainGenerator.h"
#include "TerrainRaycast.h"
#include "PlacementGrid.h"
#include <iostream>
void PlacementSystem::Update(entt::registry& registry, entt::entity player, Camera3D& camera,
    ItemId selectedItem, const TerrainSettings& terrainSettings,
    float mouseX, float mouseY, float screenWidth, float screenHeight, float aspect, PlacementGrid& placementGrid) {
    const PlaceableDef* def = PlaceableDatabase::TryGet(selectedItem);
    if (!def) {
        CancelPlacement(registry);
        return;
    }

    glm::vec3 rayOrigin = camera.GetEyePosition();
    glm::vec3 rayDir = camera.ScreenPointToRay(mouseX, mouseY, screenWidth, screenHeight, aspect);
    glm::vec3 hitPoint = TerrainRaycast::RaycastToTerrain(rayOrigin, rayDir, terrainSettings);

    float gridSize = 1.0f;   // tune — match your terrain's cellSize for visually clean alignment
    glm::vec3 ghostPos = SnapToGrid(hitPoint, gridSize);
    GridCoord coord = PlacementGrid::WorldToGrid(ghostPos, gridSize);
    glm::vec3 boundsHalfExtents = def->footprintHalfExtents; // you'll need this on PlaceableDef, see below
    auto coveredCells = PlacementGrid::GetCoveredCells(ghostPos, boundsHalfExtents, gridSize);
    bool blocked = placementGrid.IsAreaOccupied(coveredCells);
    std::cout << "Ghost at grid (" << coord.x << "," << coord.z << ") blocked=" << blocked << std::endl;


    glm::vec3 facing = GetFacingFromRotation();
    float angle = atan2(facing.x, facing.z);

    if (m_GhostEntity == entt::null || m_PendingItem != selectedItem) {
        CancelPlacement(registry);

        m_GhostEntity = registry.create();
        auto &ghostTransform = registry.emplace<TransformComponent>(m_GhostEntity);
        ghostTransform.Position = ghostPos;
        ghostTransform.Rotation = glm::angleAxis(angle, glm::vec3(0, 1, 0));

        auto mesh = ItemDatabase::GetWorldMesh(selectedItem);
        if (mesh) registry.emplace<MeshComponent>(m_GhostEntity, mesh);
        registry.emplace<GhostComponent>(m_GhostEntity, GhostComponent{ blocked }); // CHANGED — pass blocked in

         if (selectedItem == ItemId::Belt) { // ADD THIS BLOCK
            registry.emplace<BeltComponent>(m_GhostEntity, BeltComponent{ facing });
        }

        m_PendingItem = selectedItem;
    } else {
        auto &ghostTransform = registry.get<TransformComponent>(m_GhostEntity);
        ghostTransform.Position = ghostPos;
        ghostTransform.Rotation = glm::angleAxis(angle, glm::vec3(0, 1, 0)); // keep updating rotation live too, so R has immediate visual feedback
        registry.get<GhostComponent>(m_GhostEntity).blocked = blocked; // ADD THIS LINE — update every frame

        if (registry.any_of<BeltComponent>(m_GhostEntity)) { // NEW — keep facing updated live as the player rotates with R
            registry.get<BeltComponent>(m_GhostEntity).direction = facing;
        }

    }
}

void PlacementSystem::TryConfirmPlacement(entt::registry &registry, entt::entity player, ResourceMap &resourceMap, PlacementGrid &placementGrid) {
    float gridSize = 1.0f; // tune — match your terrain's cellSize for visually clean alignment
    if (m_GhostEntity == entt::null) return;

    const PlaceableDef *def = PlaceableDatabase::TryGet(m_PendingItem);
    if (!def) return; // shouldn't happen if we got this far, but guards against a stale/invalid ghost

    glm::vec3 ghostPos = registry.get<TransformComponent>(m_GhostEntity).Position;
    glm::vec3 facing = GetFacingFromRotation();

    auto coveredCells = PlacementGrid::GetCoveredCells(ghostPos, def->footprintHalfExtents, gridSize);

    if (placementGrid.IsAreaOccupied(coveredCells)) {
        return; // blocked — refuse before touching inventory at all
    }
    auto &inventory = registry.get<InventoryComponent>(player);
    for (auto &slot : inventory.slots) {
        if (slot.item == m_PendingItem && slot.count > 0) {
            slot.count--;
            if (slot.count == 0) slot.item = ItemId::None;

            placementGrid.RegisterArea(coveredCells, m_GhostEntity);

            if (m_PendingItem == ItemId::Miner) {
                ResourceCell *cell = resourceMap.GetCellAtWorldPos(ghostPos.x, ghostPos.z);

                MinerComponent miner;
                if (cell && cell->resource != ItemId::None) {
                    miner.outputItem = cell->resource;
                }
                registry.emplace<MinerComponent>(m_GhostEntity, miner);
            } else if (m_PendingItem == ItemId::Furnace) {
                registry.emplace<FurnaceComponent>(m_GhostEntity);
                registry.emplace<MachineInventoryComponent>(m_GhostEntity, MachineInventoryComponent{
                                                                                   { MachineSlot{} },
                                                                                   { MachineSlot{} } });
            } else if (m_PendingItem == ItemId::Assembler) {
                registry.emplace<AssemblerComponent>(m_GhostEntity);
                registry.emplace<MachineInventoryComponent>(m_GhostEntity, MachineInventoryComponent{
                                                                                   { MachineSlot{}, MachineSlot{} },
                                                                                   { MachineSlot{} } });
            } else if (m_PendingItem == ItemId::Belt) {
                registry.emplace_or_replace<BeltComponent>(m_GhostEntity, BeltComponent{ facing }); // safe whether or not it already exists
            } else if (m_PendingItem == ItemId::Inserter) {
                registry.emplace<InserterComponent>(m_GhostEntity, InserterComponent{ facing });
            }

            registry.emplace<BoundsComponent>(m_GhostEntity, BoundsComponent{ def->footprintHalfExtents });

            registry.remove<GhostComponent>(m_GhostEntity);
            m_GhostEntity = entt::null;
            m_PendingItem = ItemId::None;
            m_RotationSteps = 0;
            return;
        }
    }
}

void PlacementSystem::CancelPlacement(entt::registry& registry) {
    if (m_GhostEntity != entt::null && registry.valid(m_GhostEntity)) {
        registry.destroy(m_GhostEntity);
    }
    m_GhostEntity = entt::null;
    m_PendingItem = ItemId::None;
    m_RotationSteps = 0;
}
glm::vec3 PlacementSystem::SnapToGrid(glm::vec3 pos, float gridSize) {   // ADD PlacementSystem:: qualifier
    return glm::vec3(
        std::round(pos.x / gridSize) * gridSize,
        pos.y,
        std::round(pos.z / gridSize) * gridSize
    );
}
void PlacementSystem::RotateGhost() {
    m_RotationSteps = (m_RotationSteps + 1) % 4;
}

glm::vec3 PlacementSystem::GetFacingFromRotation() const {
    switch (m_RotationSteps) {
    case 0: return glm::vec3(1, 0, 0);
    case 1: return glm::vec3(0, 0, 1);
    case 2: return glm::vec3(-1, 0, 0);
    case 3: return glm::vec3(0, 0, -1);
    }
    return glm::vec3(1, 0, 0);
}