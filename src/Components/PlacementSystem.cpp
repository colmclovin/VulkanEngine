// PlacementSystem.cpp
#include "PlacementSystem.h"
#include "Components.h"
#include "../Game/PlaceableDatabase.h"
#include "../Components/ModelLoader.h"
#include "TerrainGenerator.h"
#include "TerrainRaycast.h"
void PlacementSystem::Update(entt::registry& registry, entt::entity player, Camera3D& camera,
    ItemId selectedItem, const TerrainSettings& terrainSettings,
    float mouseX, float mouseY, float screenWidth, float screenHeight, float aspect) {
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

    if (m_GhostEntity == entt::null || m_PendingItem != selectedItem) {
        CancelPlacement(registry);

        m_GhostEntity = registry.create();
        registry.emplace<TransformComponent>(m_GhostEntity).Position = ghostPos;
        auto mesh = ItemDatabase::GetWorldMesh(selectedItem);   // reuse the centralized lookup from earlier
        if (mesh) registry.emplace<MeshComponent>(m_GhostEntity, mesh);
        registry.emplace<GhostComponent>(m_GhostEntity);
        m_PendingItem = selectedItem;
    }
    else {
        registry.get<TransformComponent>(m_GhostEntity).Position = ghostPos;
    }
}

void PlacementSystem::TryConfirmPlacement(entt::registry& registry, entt::entity player, ResourceMap& resourceMap, PlacementGrid& placementGrid ) {
    if (m_GhostEntity == entt::null) return;

    auto& inventory = registry.get<InventoryComponent>(player);

    // Check the player actually has the item, consume one
    for (auto& slot : inventory.slots) {
        if (slot.item == m_PendingItem && slot.count > 0) {
            slot.count--;
            if (slot.count == 0) slot.item = ItemId::None;

            glm::vec3 ghostPos = registry.get<TransformComponent>(m_GhostEntity).Position; // ADD THIS — read it off the ghost's actual transform
            float gridSize = 1.0f; // ADD THIS — same value you already use in Update() for SnapToGrid
            GridCoord coord = PlacementGrid::WorldToGrid(ghostPos, gridSize); // ghostPos = the position you already computed
            placementGrid.Register(coord, m_GhostEntity); // needs PlacementGrid& passed into this function now

            // TODO: emplace whatever "this is a real placed building" component you want here
            if (m_PendingItem == ItemId::Miner) {
                auto pos = registry.get<TransformComponent>(m_GhostEntity).Position;
                ResourceCell *cell = resourceMap.GetCellAtWorldPos(pos.x, pos.z);

                MinerComponent miner;
                if (cell && cell->resource != ItemId::None) {
                    miner.outputItem = cell->resource;
                }
                registry.emplace<MinerComponent>(m_GhostEntity, miner);
                registry.emplace<BoundsComponent>(m_GhostEntity, BoundsComponent{ glm::vec3(1.0f, 1.0f, 1.0f) });
            } else if (m_PendingItem == ItemId::Furnace) {
                registry.emplace<FurnaceComponent>(m_GhostEntity);
                registry.emplace<MachineInventoryComponent>(m_GhostEntity, MachineInventoryComponent{
                                                                                   { MachineSlot{} }, // one input slot
                                                                                   { MachineSlot{} } // one output slot
                                                                           });
                registry.emplace<BoundsComponent>(m_GhostEntity, BoundsComponent{ glm::vec3(1.0f) });

            } else if (m_PendingItem == ItemId::Assembler) {
                registry.emplace<AssemblerComponent>(m_GhostEntity);
                registry.emplace<MachineInventoryComponent>(m_GhostEntity, MachineInventoryComponent{
                                                                                   { MachineSlot{}, MachineSlot{} }, // a couple input slots for multi-ingredient recipes
                                                                                   { MachineSlot{} } });
                registry.emplace<BoundsComponent>(m_GhostEntity, BoundsComponent{ glm::vec3(1.0f) });

            } else if (m_PendingItem == ItemId::Belt) {
                registry.emplace<BeltComponent>(m_GhostEntity); // direction defaults to +X; you'll want rotation control eventually
                registry.emplace<BoundsComponent>(m_GhostEntity, BoundsComponent{ glm::vec3(1.0f) });
            } else if (m_PendingItem == ItemId::Inserter) {
                registry.emplace<InserterComponent>(m_GhostEntity);
                registry.emplace<BoundsComponent>(m_GhostEntity, BoundsComponent{ glm::vec3(1.0f) });
            }
            // Promote the ghost into a real placed entity
            registry.remove<GhostComponent>(m_GhostEntity);
            m_GhostEntity = entt::null;
            m_PendingItem = ItemId::None;
            return;
        }
    }
    // No item available — placement fails silently, or you could feed back to the player here
}

void PlacementSystem::CancelPlacement(entt::registry& registry) {
    if (m_GhostEntity != entt::null && registry.valid(m_GhostEntity)) {
        registry.destroy(m_GhostEntity);
    }
    m_GhostEntity = entt::null;
    m_PendingItem = ItemId::None;
}

/*glm::vec3 PlacementSystem::RaycastToTerrain(glm::vec3 rayOrigin, glm::vec3 rayDir, const TerrainSettings& terrainSettings) {
    // March along the ray in small steps, checking when it crosses the terrain height at that XZ.
    // Simple and robust for a heightmap; avoids needing real mesh-triangle intersection.
    const float stepSize = 0.25f;
    const float maxDistance = 200.0f;

    glm::vec3 pos = rayOrigin;
    for (float t = 0.0f; t < maxDistance; t += stepSize) {
        pos = rayOrigin + rayDir * t;
        float terrainY = TerrainGenerator::SampleHeight(pos.x, pos.z, terrainSettings);
        if (pos.y <= terrainY) {
            return glm::vec3(pos.x, terrainY, pos.z);   // hit — snap Y exactly to terrain surface
        }
    }
    return rayOrigin + rayDir * maxDistance;   // no hit within range — fallback point far along the ray
}
*/
glm::vec3 PlacementSystem::SnapToGrid(glm::vec3 pos, float gridSize) {   // ADD PlacementSystem:: qualifier
    return glm::vec3(
        std::round(pos.x / gridSize) * gridSize,
        pos.y,
        std::round(pos.z / gridSize) * gridSize
    );
}