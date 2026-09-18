// PowerSystem.cpp
#include "PowerSystem.h"
#include "Components.h"
#include "../Game/FuelDatabase.h"
#include "PowerComponent.h"
#include <unordered_set>
#include <vector>

void PowerSystem::Update(entt::registry &registry, float deltaTime) {
    auto genView = registry.view<TransformComponent, PowerGeneratorComponent>();
    auto poleView = registry.view<TransformComponent, PowerPoleComponent>();
    auto consumerView = registry.view<TransformComponent, PowerConsumerComponent>();

    // Reset all consumers to unpowered first
    for (auto entity : consumerView) {
        consumerView.get<PowerConsumerComponent>(entity).isPowered = false;
    }

    for (auto genEntity : genView) {
        auto &gen = genView.get<PowerGeneratorComponent>(genEntity);
        auto &genTransform = genView.get<TransformComponent>(genEntity);

        bool canProduce = gen.fuelRemaining > 0.0f || gen.fuelBuffer > 0;
        if (!canProduce) continue;

        // Flood-fill poles reachable from this generator
        std::unordered_set<entt::entity> reachablePoles;
        std::vector<entt::entity> frontier;

        for (auto poleEntity : poleView) {
            auto &poleTransform = poleView.get<TransformComponent>(poleEntity);
            auto &pole = poleView.get<PowerPoleComponent>(poleEntity);
            if (glm::length(poleTransform.Position - genTransform.Position) <= pole.radius) {
                reachablePoles.insert(poleEntity);
                frontier.push_back(poleEntity);
            }
        }
        while (!frontier.empty()) {
            entt::entity current = frontier.back();
            frontier.pop_back();
            auto &curTransform = poleView.get<TransformComponent>(current);
            auto &curPole = poleView.get<PowerPoleComponent>(current);
            for (auto poleEntity : poleView) {
                if (reachablePoles.count(poleEntity)) continue;
                auto &poleTransform = poleView.get<TransformComponent>(poleEntity);
                auto &pole = poleView.get<PowerPoleComponent>(poleEntity);
                float dist = glm::length(poleTransform.Position - curTransform.Position);
                if (dist <= curPole.radius || dist <= pole.radius) {
                    reachablePoles.insert(poleEntity);
                    frontier.push_back(poleEntity);
                }
            }
        }

        // Does any reachable consumer actually want power right now?
        bool anyDemand = false;
        for (auto poleEntity : reachablePoles) {
            auto &poleTransform = poleView.get<TransformComponent>(poleEntity);
            auto &pole = poleView.get<PowerPoleComponent>(poleEntity);
            for (auto consumerEntity : consumerView) {
                auto &consumer = consumerView.get<PowerConsumerComponent>(consumerEntity);
                if (!consumer.wantsPower) continue;
                auto &consumerTransform = consumerView.get<TransformComponent>(consumerEntity);
                if (glm::length(consumerTransform.Position - poleTransform.Position) <= pole.radius) {
                    anyDemand = true;
                    break;
                }
            }
            if (anyDemand) break;
        }

        if (!anyDemand) continue; // no demand on this network — skip fuel consumption entirely

        if (gen.fuelRemaining <= 0.0f) {
            const FuelDef *fuelDef = FuelDatabase::TryGet(gen.loadedFuelType);
            if (!fuelDef) continue;
            gen.fuelBuffer--;
            gen.fuelRemaining = fuelDef->burnTime;
            if (gen.fuelBuffer == 0) gen.loadedFuelType = ItemId::None;
        }
        gen.fuelRemaining -= deltaTime;

        for (auto poleEntity : reachablePoles) {
            auto &poleTransform = poleView.get<TransformComponent>(poleEntity);
            auto &pole = poleView.get<PowerPoleComponent>(poleEntity);
            for (auto consumerEntity : consumerView) {
                auto &consumerTransform = consumerView.get<TransformComponent>(consumerEntity);
                if (glm::length(consumerTransform.Position - poleTransform.Position) <= pole.radius) {
                    consumerView.get<PowerConsumerComponent>(consumerEntity).isPowered = true;
                }
            }
        }
    }
}