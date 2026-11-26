#include "CombatSystem.h"
#include "DamageSystem.h"
#include "../Components/AttackOrderComponent.h"
#include "../Components/CombatStatsComponent.h"
#include "../Components/HealthComponent.h"
#include "../Components/MoveOrderComponent.h"
#include "../Components/StanceComponent.h"
#include "../Components/TeamComponent.h"
#include "../Components/TransformComponent.h"

static float DistXZ(const glm::vec3 &a, const glm::vec3 &b) {
    float dx = a.x - b.x, dz = a.z - b.z;
    return std::sqrt(dx * dx + dz * dz);
}

static entt::entity FindNearestEnemy(entt::registry &reg, entt::entity self, uint8_t team,
                                     const glm::vec3 &pos, float range) {
    entt::entity best = entt::null;
    float bestD = range;
    auto v = reg.view<TransformComponent, TeamComponent, HealthComponent>();
    for (auto e : v) {
        if (e == self || v.get<TeamComponent>(e).team == team) continue;
        float d = DistXZ(pos, v.get<TransformComponent>(e).Position);
        if (d <= bestD) {
            bestD = d;
            best = e;
        }
    }
    return best;
}

void CombatSystem::Update(entt::registry &reg, float dt, std::vector<Shot> &shots) {
    auto view = reg.view<CombatStatsComponent, TransformComponent, TeamComponent>();
    for (auto e : view) {
        auto &stats = view.get<CombatStatsComponent>(e);
        glm::vec3 pos = view.get<TransformComponent>(e).Position;
        uint8_t team = view.get<TeamComponent>(e).team;

        if (stats.cooldown > 0.0f) stats.cooldown -= dt;

        entt::entity target = entt::null;
        bool chasing = false;

        if (auto *order = reg.try_get<AttackOrderComponent>(e)) {
            if (reg.valid(order->target) && reg.all_of<HealthComponent, TransformComponent>(order->target)) {
                target = order->target;
                chasing = true;
            } else {
                reg.remove<AttackOrderComponent>(e); // target died or vanished
            }
        } else {
            bool moving = reg.all_of<MoveOrderComponent>(e);
            auto *st = reg.try_get<StanceComponent>(e);
            bool willFire = st && st->stance == Stance::FireAtWill;
            if (willFire && (!moving || stats.fireWhileMoving))
                target = FindNearestEnemy(reg, e, team, pos, stats.range);
        }

        if (target == entt::null) continue;

        glm::vec3 tpos = reg.get<TransformComponent>(target).Position;
        float d = DistXZ(pos, tpos);

        if (chasing) {
            if (d > stats.range) {
                reg.emplace_or_replace<MoveOrderComponent>(e, MoveOrderComponent{ tpos });
                continue; // still closing in
            }
            reg.remove<MoveOrderComponent>(e); // in range: stop and shoot
        }

        if (d <= stats.range && stats.cooldown <= 0.0f) {
            DamageSystem::ApplyDamage(reg, target, stats.damage, stats.damageType, e);
            stats.cooldown = stats.attackInterval;
            shots.push_back({ pos + glm::vec3(0, 1.2f, 0), tpos + glm::vec3(0, 1.2f, 0) });
        }
    }
}