// PowerSystem.cpp
#include "PowerSystem.h"
#include "Components.h"
#include "../Game/FuelDatabase.h"
#include "PowerComponent.h"
#include <unordered_set>
#include <vector>

void PowerSystem::Update(entt::registry &registry, float deltaTime) {
    // 1. Tick generators, determine which are actively producing power this frame
    auto genView = registry.view<TransformComponent, PowerGeneratorComponent>();
    std::vector<entt::entity> activeGenerators;

    for (auto entity : genView) {
        auto &gen = genView.get<PowerGeneratorComponent>(entity);

        if (gen.fuelRemaining <= 0.0f) {
            if (gen.fuelBuffer <= 0) continue;
            const FuelDef *fuelDef = FuelDatabase::TryGet(gen.loadedFuelType);
            if (!fuelDef) continue;
            gen.fuelBuffer--;
            gen.fuelRemaining = fuelDef->burnTime;
            if (gen.fuelBuffer == 0) gen.loadedFuelType = ItemId::None;
        }
        gen.fuelRemaining -= deltaTime;
        activeGenerators.push_back(entity);
    }

    // 2. Reset all consumers to unpowered
    auto consumerView = registry.view<PowerConsumerComponent>();
    for (auto entity : consumerView) {
        consumerView.get<PowerConsumerComponent>(entity).isPowered = false;
    }

    if (activeGenerators.empty()) return;

    // 3. Flood-fill from each active generator through connected poles
    auto poleView = registry.view<TransformComponent, PowerPoleComponent>();
    std::unordered_set<entt::entity> poweredPoles;
    std::vector<entt::entity> frontier;

    auto genTransformView = registry.view<TransformComponent, PowerGeneratorComponent>();
    for (auto genEntity : activeGenerators) {
        glm::vec3 genPos = genTransformView.get<TransformComponent>(genEntity).Position;

        for (auto poleEntity : poleView) {
            if (poweredPoles.count(poleEntity)) continue;
            auto &poleTransform = poleView.get<TransformComponent>(poleEntity);
            auto &pole = poleView.get<PowerPoleComponent>(poleEntity);

            if (glm::length(poleTransform.Position - genPos) <= pole.radius) {
                poweredPoles.insert(poleEntity);
                frontier.push_back(poleEntity);
            }
        }
    }

    // Expand: poles powering other poles within range
    while (!frontier.empty()) {
        entt::entity current = frontier.back();
        frontier.pop_back();
        auto &curTransform = poleView.get<TransformComponent>(current);
        auto &curPole = poleView.get<PowerPoleComponent>(current);

        for (auto poleEntity : poleView) {
            if (poweredPoles.count(poleEntity)) continue;
            auto &poleTransform = poleView.get<TransformComponent>(poleEntity);
            auto &pole = poleView.get<PowerPoleComponent>(poleEntity);

            float dist = glm::length(poleTransform.Position - curTransform.Position);
            if (dist <= curPole.radius || dist <= pole.radius) {
                poweredPoles.insert(poleEntity);
                frontier.push_back(poleEntity);
            }
        }
    }

    // 4. Mark consumers within range of any powered pole
    for (auto poleEntity : poweredPoles) {
        auto &poleTransform = poleView.get<TransformComponent>(poleEntity);
        auto &pole = poleView.get<PowerPoleComponent>(poleEntity);

        for (auto consumerEntity : consumerView) {
            auto &consumerTransform = registry.get<TransformComponent>(consumerEntity);
            if (glm::length(consumerTransform.Position - poleTransform.Position) <= pole.radius) {
                consumerView.get<PowerConsumerComponent>(consumerEntity).isPowered = true;
            }
        }
    }
}