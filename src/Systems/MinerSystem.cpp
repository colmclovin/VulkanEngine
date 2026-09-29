#include "MinerSystem.h"
#include "../Components/Components.h"
#include "../Components/InventoryComponent.h"
#include "../Components/MinerComponent.h"
#include "../Game/FuelDatabase.h"


void MinerSystem::Update(entt::registry& registry, DepletionMap& depletionMap, const TerrainSettings& terrainSettings, float deltaTime, ChunkManager& m_ChunkManager) {
    auto view = registry.view<TransformComponent, MinerComponent>();

    for (auto entity : view) {
        auto& transform = view.get<TransformComponent>(entity);
        auto& miner = view.get<MinerComponent>(entity);

        if (miner.outputItem == ItemId::None) continue;
        if (registry.any_of<PowerConsumerComponent>(entity)) {
            auto& consumer = registry.get<PowerConsumerComponent>(entity);
            consumer.wantsPower = (miner.outputItem != ItemId::None && miner.outputBuffer < miner.outputBufferCapacity);
        }
        if (miner.outputBuffer >= miner.outputBufferCapacity) continue;

        bool hasPower = false;
        if (registry.any_of<PowerConsumerComponent>(entity)) {
            hasPower = registry.get<PowerConsumerComponent>(entity).isPowered;
        }
        miner.runningOnPower = hasPower;

        

        if (!hasPower) {
            if (miner.fuelRemaining <= 0.0f) {
                if (miner.fuelBuffer <= 0) continue;

                const FuelDef* fuelDef = FuelDatabase::TryGet(miner.loadedFuelType);
                if (!fuelDef) continue;

                miner.fuelBuffer--;
                miner.fuelRemaining = fuelDef->burnTime;
                if (miner.fuelBuffer == 0) miner.loadedFuelType = ItemId::None;
            }
            miner.fuelRemaining -= deltaTime;
        }

        miner.outputTimer += deltaTime;
        if (miner.outputTimer >= miner.outputInterval) {
            miner.outputTimer -= miner.outputInterval;

            auto deposit = OreDepositMap::GetDepositAt(transform.Position.x, transform.Position.z, terrainSettings.seed);   // CHANGED
            if (deposit && deposit->item == miner.outputItem) {
                int cellX = static_cast<int>(std::round(transform.Position.x));
                int cellZ = static_cast<int>(std::round(transform.Position.z));
                float remaining = depletionMap.GetRemainingFraction(cellX, cellZ);

                if (remaining > 0.0f) {
                    depletionMap.Deplete(cellX, cellZ, miner.extractionRate, deposit->amount);   // CHANGED
                    miner.outputBuffer++;
                }
                else {

                    miner.outputItem = ItemId::None;   // patch fully depleted
                }
            }
            else {
                miner.outputItem = ItemId::None;
            }
        }
    }
}