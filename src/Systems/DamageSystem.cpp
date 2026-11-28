#include "DamageSystem.h"
#include "../Components/Components.h"
#include "../World/PlacementGrid.h"
#include <vector>

void DamageSystem::ApplyDamage(entt::registry &registry, entt::entity target, float amount,
                               DamageType type, entt::entity) {
    if (!registry.valid(target)) return;
    auto *hp = registry.try_get<HealthComponent>(target);
    if (!hp) return;

    float mult = 1.0f;
    if (auto *res = registry.try_get<ResistanceComponent>(target)) {
        mult = res->Get(type);
        if (type == DamageType::ArmorPiercing && mult < 1.0f)
            mult = mult + (1.0f - mult) * 0.5f; // armor piercing cuts the armor's effect by half
    }

    hp->current -= amount * mult;
}

void DamageSystem::ProcessDeaths(entt::registry &registry, const glm::vec3 &worldSpawn, PlacementGrid &grid, float cellSize) {
    // Collect first, then act, so we never destroy while iterating
    std::vector<entt::entity> dead;
    auto view = registry.view<HealthComponent>();
    for (auto e : view)
        if (view.get<HealthComponent>(e).current <= 0.0f) dead.push_back(e);

    for (auto e : dead) {
        if (!registry.valid(e)) continue;

        // The player respawns instead of being destroyed (hook for step 3)
        if (registry.all_of<PlayerTag>(e)) { // use whatever marks your player; see note below
            auto &hp = registry.get<HealthComponent>(e);
            hp.current = hp.max;
            glm::vec3 spawn = worldSpawn;
            auto cv = registry.view<CatalystComponent, TransformComponent, TeamComponent>();
            for (auto c : cv) {
                if (cv.get<TeamComponent>(c).team == 0) {
                    spawn = cv.get<TransformComponent>(c).Position + glm::vec3(5.0f, 0.0f, 0.0f);
                    break;
                }
            }
            registry.get<TransformComponent>(e).Position = spawn;
            registry.remove<MoveOrderComponent>(e);
            continue;
        }

        // The Catalyst dying is a loss for that team (hook for step 3)
        if (registry.all_of<CatalystComponent>(e)) {
            // step 3 raises the game-over event here
        }
        if (!registry.all_of<CombatStatsComponent>(e)) {
            if (auto *t = registry.try_get<TransformComponent>(e)) {
                if (auto *b = registry.try_get<BoundsComponent>(e)) {
                    auto cells = PlacementGrid::GetCoveredCells(t->Position, b->halfExtents, cellSize);
                    for (auto &c : cells)
                        grid.Unregister(c);
                }
            }
        }
        // Placed buildings also need unregistering from the PlacementGrid; step 2 adds that
        registry.destroy(e);
    }
}