#include "MoveOrderSystem.h"
#include "../Components/Components.h"
#include "../Utils/GpuNoiseMatch.h"


void MoveOrderSystem::Update(entt::registry& registry, float deltaTime, const TerrainSettings& settings) {
    auto view = registry.view<TransformComponent, MoveOrderComponent>();
    for (auto entity : view) {
        auto& transform = view.get<TransformComponent>(entity);
        auto& order = view.get<MoveOrderComponent>(entity);

        glm::vec3 toTarget = order.targetPosition - transform.Position;
        toTarget.y = 0.0f;   // ignore height difference for movement direction
        float dist = glm::length(toTarget);

        const float ARRIVE_THRESHOLD = 0.5f;
        if (dist < ARRIVE_THRESHOLD) {
            registry.remove<MoveOrderComponent>(entity);
            continue;
        }

        glm::vec3 moveDir = glm::normalize(toTarget);
        float speed = 10.0f;
        if (auto *cs = registry.try_get<CombatStatsComponent>(entity)) speed = cs->moveSpeed;
        float step = speed * deltaTime;
        if (dist <= std::max(step, ARRIVE_THRESHOLD)) {
            transform.Position.x = order.targetPosition.x;
            transform.Position.z = order.targetPosition.z;
            transform.Position.y = GpuNoiseMatch::SampleHeight(transform.Position.x, transform.Position.z, static_cast<float>(settings.seed));
            registry.remove<MoveOrderComponent>(entity);
            continue;
        }
        transform.Position += moveDir * step;
    }
}