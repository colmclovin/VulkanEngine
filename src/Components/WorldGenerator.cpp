#include "WorldGenerator.h"
#include "TerrainGenerator.h"
#include "ModelLoader.h"
#include "Components.h"
#include "HarvestableComponent.h"
#include "../Game/ItemDatabase.h"
#include <FastNoiseLite.h>
#include <cstdlib>
#include "BoundsComponent.h"
#include <iostream>

static void SpawnTree(entt::registry& registry, glm::vec3 pos, std::shared_ptr<Mesh> treeMesh) {
    auto entity = registry.create();
    auto& transform = registry.emplace<TransformComponent>(entity);
    transform.Position = pos;

    registry.emplace<MeshComponent>(entity, treeMesh);
    registry.emplace<BoundsComponent>(entity, BoundsComponent{ glm::vec3(0.5f, 2.0f, 0.5f) }); // ADD THIS — tune to your tree model's actual size
    HarvestableComponent harvest;
    harvest.health = 100.0f;
    harvest.maxHealth = 100.0f;
    harvest.yieldItem = ItemId::Wood;
    harvest.yieldPerHit = 1;
    harvest.yieldOnDestroy = 5;
    registry.emplace<HarvestableComponent>(entity, harvest);

    registry.emplace<NameTag>(entity, "Tree");
}

void WorldGenerator::ScatterTrees(entt::registry &registry, const TerrainSettings &terrainSettings, ResourceMap &resourceMap) {
    int totalSamples = 0, forestSamples = 0, treesSpawned = 0;
    FastNoiseLite treeDensityNoise;
    treeDensityNoise.SetSeed(terrainSettings.seed + 4000);
    treeDensityNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    treeDensityNoise.SetFrequency(0.4f); // fine detail for "which exact spots have a tree"

    float worldWidth = terrainSettings.gridWidth * terrainSettings.cellSize;
    float worldDepth = terrainSettings.gridDepth * terrainSettings.cellSize;
    float sampleSpacing = 2.0f; // denser sampling than before, since not every sample spawns a tree

    auto treeMesh = std::make_shared<Mesh>(ModelLoader::LoadModel("Assets/Models/Tree.glb"));

    for (float x = 0.0f; x < worldWidth; x += sampleSpacing) {
        for (float z = 0.0f; z < worldDepth; z += sampleSpacing) {
            totalSamples++;
            RegionType region = resourceMap.GetRegionAtWorldPos(x, z, terrainSettings.seed);
            if (region != RegionType::Forest) continue; // only place trees inside forest regions
            forestSamples++;

            float density = treeDensityNoise.GetNoise(x, z);
            if (density > 0.3f) { // sparse — only a fraction of forest tiles actually get a tree
                float jitterX = (rand() / (float)RAND_MAX - 0.5f) * sampleSpacing;
                float jitterZ = (rand() / (float)RAND_MAX - 0.5f) * sampleSpacing;
                float worldX = x + jitterX;
                float worldZ = z + jitterZ;
                float worldY = TerrainGenerator::SampleHeight(worldX, worldZ, terrainSettings);
                treesSpawned++;
                SpawnTree(registry, glm::vec3(worldX, worldY, worldZ), treeMesh);
            }
        }
    }
    std::cout << "Total samples: " << totalSamples << "  Forest samples: " << forestSamples << "  Trees spawned: " << treesSpawned << std::endl;
}

void WorldGenerator::ClearHarvestables(entt::registry& registry) {
    auto view = registry.view<HarvestableComponent>();
    registry.destroy(view.begin(), view.end());
}