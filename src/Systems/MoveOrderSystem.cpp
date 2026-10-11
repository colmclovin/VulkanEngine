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
        float moveSpeed = 10.0f;   // tune, or pull from a PlayerComponent/UnitComponent's own speed
        transform.Position += moveDir * moveSpeed * deltaTime;
        transform.Position.y = GpuNoiseMatch::SampleHeight(transform.Position.x, transform.Position.z, static_cast<float>(settings.seed));   // ground-snap
    }
}