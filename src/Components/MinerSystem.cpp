#include "MinerSystem.h"
#include "Components.h"
#include "InventoryComponent.h"
#include "MinerComponent.h"

void MinerSystem::Update(entt::registry &registry, ResourceMap &resourceMap, float deltaTime) {
    auto view = registry.view<TransformComponent, MinerComponent>();

    for (auto entity : view) {
        auto &transform = view.get<TransformComponent>(entity);
        auto &miner = view.get<MinerComponent>(entity);

        if (miner.outputItem == ItemId::None) continue; // nothing to mine here
        if (miner.outputBuffer >= miner.outputBufferCapacity) continue; // buffer full, wait for collection

        // --- Fuel handling ---
        if (miner.fuelRemaining <= 0.0f) {
            if (miner.fuelBuffer > 0) {
                miner.fuelBuffer--;
                miner.fuelRemaining = miner.fuelBurnTime;
            } else {
                continue; // out of fuel, idle
            }
        }
        miner.fuelRemaining -= deltaTime;

        // --- Extraction ---
        miner.outputTimer += deltaTime;
        if (miner.outputTimer >= miner.outputInterval) {
            miner.outputTimer -= miner.outputInterval;

            // Pull from the ResourceMap cell(s) within collectionRadius.
            // Simplest version: just sample the miner's own position — expand to a small area scan later if needed.
            ResourceCell *cell = resourceMap.GetCellAtWorldPos(transform.Position.x, transform.Position.z);
            if (cell && cell->resource == miner.outputItem && cell->amount > 0.0f) {
                resourceMap.ExtractFromCell(cell, miner.extractionRate);
                miner.outputBuffer++;
            } else {
                miner.outputItem = ItemId::None; // patch depleted or gone — miner goes idle permanently
            }
        }
    }
}