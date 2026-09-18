#include "MinerSystem.h"
#include "Components.h"
#include "InventoryComponent.h"
#include "MinerComponent.h"
#include "../Game/FuelDatabase.h"

void MinerSystem::Update(entt::registry &registry, ResourceMap &resourceMap, float deltaTime) {
    auto view = registry.view<TransformComponent, MinerComponent>();

    for (auto entity : view) {
        auto &transform = view.get<TransformComponent>(entity);
        auto &miner = view.get<MinerComponent>(entity);

        if (miner.outputItem == ItemId::None) continue;
        if (miner.outputBuffer >= miner.outputBufferCapacity) continue;

        bool hasPower = false;
        if (registry.any_of<PowerConsumerComponent>(entity)) {
            hasPower = registry.get<PowerConsumerComponent>(entity).isPowered;
        }
        miner.runningOnPower = hasPower;

        if (registry.any_of<PowerConsumerComponent>(entity)) {
            auto &consumer = registry.get<PowerConsumerComponent>(entity);
            consumer.wantsPower = (miner.outputItem != ItemId::None && miner.outputBuffer < miner.outputBufferCapacity);
        }

        if (!hasPower) {
            // Fuel path — unchanged from before
            if (miner.fuelRemaining <= 0.0f) {
                if (miner.fuelBuffer <= 0) continue;

                const FuelDef *fuelDef = FuelDatabase::TryGet(miner.loadedFuelType);
                if (!fuelDef) continue;

                miner.fuelBuffer--;
                miner.fuelRemaining = fuelDef->burnTime;
                if (miner.fuelBuffer == 0) miner.loadedFuelType = ItemId::None;
            }
            miner.fuelRemaining -= deltaTime;
        }
        // if hasPower, no fuel is touched at all — PowerSystem handles whether the generator can sustain this draw

        miner.outputTimer += deltaTime;
        if (miner.outputTimer >= miner.outputInterval) {
            miner.outputTimer -= miner.outputInterval;

            ResourceCell *cell = resourceMap.GetCellAtWorldPos(transform.Position.x, transform.Position.z);
            if (cell && cell->resource == miner.outputItem && cell->amount > 0.0f) {
                resourceMap.ExtractFromCell(cell, miner.extractionRate);
                miner.outputBuffer++;
            } else {
                miner.outputItem = ItemId::None;
            }
        }
    }
}