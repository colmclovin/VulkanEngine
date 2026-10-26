#include "DamageSystem.h"
#include "../Components/Components.h"
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

void DamageSystem::ProcessDeaths(entt::registry &registry, const glm::vec3 &worldSpawn) {
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
            registry.get<TransformComponent>(e).Position = worldSpawn; // step 3 replaces this with the Catalyst position
            registry.remove<MoveOrderComponent>(e);
            continue;
        }

        // The Catalyst dying is a loss for that team (hook for step 3)
        if (registry.all_of<CatalystComponent>(e)) {
            // step 3 raises the game-over event here
        }

        // Placed buildings also need unregistering from the PlacementGrid; step 2 adds that
        registry.destroy(e);
    }
}