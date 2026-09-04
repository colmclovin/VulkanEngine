#include "WorldGenerator.h"
#include "TerrainGenerator.h"
#include "ModelLoader.h"
#include "Components.h"
#include "HarvestableComponent.h"
#include "../Game/ItemDatabase.h"
#include <FastNoiseLite.h>
#include <cstdlib>



static void SpawnTree(entt::registry& registry, glm::vec3 pos, std::shared_ptr<Mesh> treeMesh) {
    auto entity = registry.create();
    auto& transform = registry.emplace<TransformComponent>(entity);
    transform.Position = pos;

    registry.emplace<MeshComponent>(entity, treeMesh);

    HarvestableComponent harvest;
    harvest.health = 100.0f;
    harvest.maxHealth = 100.0f;
    harvest.yieldItem = ItemId::Wood;
    harvest.yieldPerHit = 1;
    harvest.yieldOnDestroy = 5;
    registry.emplace<HarvestableComponent>(entity, harvest);

    registry.emplace<NameTag>(entity, "Tree");
}

void WorldGenerator::ScatterTrees(entt::registry& registry, const TerrainSettings& terrainSettings) {
    FastNoiseLite placementNoise;
    placementNoise.SetSeed(terrainSettings.seed + 2000);   // distinct offset from both height and resource noise
    placementNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    placementNoise.SetFrequency(0.15f);

    float worldWidth = terrainSettings.gridWidth * terrainSettings.cellSize;
    float worldDepth = terrainSettings.gridDepth * terrainSettings.cellSize;
    float sampleSpacing = 3.0f;

    auto treeMesh = std::make_shared<Mesh>(ModelLoader::LoadModel("Assets/Models/Tree.glb"));

    for (float x = 0.0f; x < worldWidth; x += sampleSpacing) {
        for (float z = 0.0f; z < worldDepth; z += sampleSpacing) {
            float n = placementNoise.GetNoise(x, z);
            if (n > 0.5f && n < 0.6f) {   // narrow band = sparse, scattered trees
                float jitterX = (rand() / (float)RAND_MAX - 0.5f) * sampleSpacing;
                float jitterZ = (rand() / (float)RAND_MAX - 0.5f) * sampleSpacing;
                float worldX = x + jitterX;
                float worldZ = z + jitterZ;
                float worldY = TerrainGenerator::SampleHeight(worldX, worldZ, terrainSettings);

                SpawnTree(registry, glm::vec3(worldX, worldY, worldZ), treeMesh);
            }
        }
    }
}

void WorldGenerator::ClearHarvestables(entt::registry& registry) {
    auto view = registry.view<HarvestableComponent>();
    registry.destroy(view.begin(), view.end());
}