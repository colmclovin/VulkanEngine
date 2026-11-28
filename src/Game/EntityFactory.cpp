#include "EntityFactory.h"
#include "../Components/Components.h"
#include "../Rendering/SkinnedMesh.h"
#include "../Utils/GpuNoiseMatch.h"
#include "ResistancePresets.h"
#include "UnitDatabase.h"
#include <iostream>

namespace EntityFactory {

entt::entity CreatePlayer(entt::registry &registry, const TransformComponent &transform, std::shared_ptr<SkinnedMesh> mesh) {
    entt::entity player = registry.create();
    registry.emplace<TransformComponent>(player, transform);
    registry.emplace<PlayerComponent>(player);
    registry.emplace<PlayerTag>(player);
    registry.emplace<NameTag>(player, "Player");
    registry.emplace<InventoryComponent>(player);
    registry.emplace<SelectableComponent>(player);
    registry.emplace<BoundsComponent>(player, BoundsComponent{ HUMANOID_HALF_EXTENTS });
    registry.emplace<AnimationComponent>(player);
    registry.emplace<SkinnedMeshComponent>(player, std::move(mesh));
    registry.emplace<VisionComponent>(player, PLAYER_VISION_RADIUS);
    registry.emplace<TeamComponent>(player, PLAYER_TEAM);
    registry.emplace<HealthComponent>(player);
    CombatStatsComponent stats;
    stats.fireWhileMoving = false;
    registry.emplace<CombatStatsComponent>(player, stats);
    registry.emplace<StanceComponent>(player);
    registry.emplace<ResistanceComponent>(player, ResistancePresets::Player());
    return player;
}

entt::entity CreateUnit(entt::registry &registry, ItemId unitItem, glm::vec3 position, uint8_t team,
                        std::shared_ptr<SkinnedMesh> mesh, int seed) {
    if (!mesh) {
        std::cerr << "CreateUnit: unit mesh is null" << std::endl;
        return entt::null;
    }
    const UnitDef *def = UnitDatabase::TryGet(unitItem);
    if (!def) {
        std::cerr << "CreateUnit: no UnitDef for item " << (int)unitItem << std::endl;
        return entt::null;
    }

    entt::entity e = registry.create();
    auto &tr = registry.emplace<TransformComponent>(e);
    tr.Position = position;
    tr.Position.y = GpuNoiseMatch::SampleSurfaceHeight(position.x, position.z, (float)seed);

    registry.emplace<AnimationComponent>(e);
    registry.emplace<SkinnedMeshComponent>(e, std::move(mesh));
    registry.emplace<BoundsComponent>(e, BoundsComponent{ HUMANOID_HALF_EXTENTS });
    registry.emplace<HealthComponent>(e, HealthComponent{ def->maxHealth, def->maxHealth });
    registry.emplace<ResistanceComponent>(e, def->resist);
    registry.emplace<TeamComponent>(e, TeamComponent{ team });
    registry.emplace<CombatStatsComponent>(e, def->stats);
    registry.emplace<UnitTypeComponent>(e, UnitTypeComponent{ unitItem });
    registry.emplace<StanceComponent>(e, StanceComponent{ team == PLAYER_TEAM ? Stance::FireAtWill : Stance::HoldFire });

    if (team == PLAYER_TEAM) {
        registry.emplace<SelectableComponent>(e);
        registry.emplace<VisionComponent>(e, VisionComponent{ def->visionRadius });
    }
    return e;
}

} // namespace EntityFactory
