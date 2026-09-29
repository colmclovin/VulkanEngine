#include "WorldGenerator.h"
#include "TerrainGenerator.h"
#include "../Rendering/ModelLoader.h"
#include "../Components/Components.h"
#include "../Components/HarvestableComponent.h"
#include "../Game/ItemDatabase.h"
#include <FastNoiseLite.h>
#include <cstdlib>
#include "../Components/BoundsComponent.h"
#include <iostream>
#include "../Engine/VulkanEngine.h"
#include <random>
#include "../Game/BiomeMap.h"
#include "../Game/BiomeDatabase.h"
#include "../Game/OreDepositMap.h"
#include "PlacementGrid.h"
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

void WorldGenerator::ScatterTrees(entt::registry &registry, const TerrainSettings &terrainSettings, ResourceMap &resourceMap, VulkanEngine *engine, MeshRenderer *meshRenderer) {
	int totalSamples = 0, forestSamples = 0, treesSpawned = 0;
	FastNoiseLite treeDensityNoise;
	treeDensityNoise.SetSeed(terrainSettings.seed + 4000);
	treeDensityNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	treeDensityNoise.SetFrequency(0.4f); // fine detail for "which exact spots have a tree"

	float worldWidth = terrainSettings.gridWidth * terrainSettings.cellSize;
	float worldDepth = terrainSettings.gridDepth * terrainSettings.cellSize;
	float sampleSpacing = 2.0f; // denser sampling than before, since not every sample spawns a tree

	auto treeMesh = std::make_shared<Mesh>(ModelLoader::LoadModel("Assets/Models/Tree.glb", engine, meshRenderer));

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

glm::vec3 WorldGenerator::FindSpawnPoint(const TerrainSettings &terrainSettings, int seed, std::mt19937 &rng) {
	float worldWidth = terrainSettings.gridWidth * terrainSettings.cellSize;
	float worldDepth = terrainSettings.gridDepth * terrainSettings.cellSize;

	std::uniform_real_distribution<float> xDist(0.0f, worldWidth); // CHANGED — match terrain's actual [0, worldWidth] range
	std::uniform_real_distribution<float> zDist(0.0f, worldDepth); // CHANGED — match terrain's actual [0, worldDepth] range

	const int maxAttempts = 200;
	for (int i = 0; i < maxAttempts; i++) {
		float x = xDist(rng);
		float z = zDist(rng);

		BiomeId biome = BiomeMap::GetBiomeAt(x, z, seed);
		if (biome == BiomeId::Lake || biome == BiomeId::Mountains) continue;

		float height = TerrainGenerator::SampleHeight(x, z, terrainSettings);
		return glm::vec3(x, height, z);
	}

	return glm::vec3(worldWidth * 0.5f, TerrainGenerator::SampleHeight(worldWidth * 0.5f, worldDepth * 0.5f, terrainSettings), worldDepth * 0.5f);
}

std::shared_ptr<Mesh> WorldGenerator::s_TreeMeshCache = nullptr;


void WorldGenerator::ScatterTreesInChunk(entt::registry &registry, ChunkCoord coord, float chunkWorldSize,
										 const TerrainSettings &terrainSettings, VulkanEngine *engine, MeshRenderer *meshRenderer,
										 std::vector<entt::entity> &outTreeEntities, PlacementGrid &placementGrid) {
	float chunkOriginX = coord.x * chunkWorldSize;
	float chunkOriginZ = coord.z * chunkWorldSize;
	float worldExtentZ = terrainSettings.gridDepth * terrainSettings.cellSize;

	FastNoiseLite treeJitterNoise; // used only for per-sample placement jitter/roll, not density itself
	treeJitterNoise.SetSeed(terrainSettings.seed + 4000);
	treeJitterNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	treeJitterNoise.SetFrequency(0.4f);

	float sampleSpacing = 2.0f;
	if (!s_TreeMeshCache) { // NEW — only load once, ever, for the lifetime of the program
		s_TreeMeshCache = std::make_shared<Mesh>(ModelLoader::LoadModel("Assets/Models/Tree.glb", engine, meshRenderer));
	}
	auto treeMesh = s_TreeMeshCache; // CHANGED — reuse the cached instance instead of reloadings
	for (float x = 0.0f; x < chunkWorldSize; x += sampleSpacing) {
		for (float z = 0.0f; z < chunkWorldSize; z += sampleSpacing) {
			float worldX = chunkOriginX + x;
			float worldZ = chunkOriginZ + z;

			BiomeId biome = BiomeMap::GetBiomeAt(worldX, worldZ, terrainSettings.seed);
			if (biome == BiomeId::Lake) continue; // hard exclusion — never any trees in lakes

			const BiomeDef &def = BiomeDatabase::Get(biome);
			if (def.treeDensity <= 0.0f) continue; // biome doesn't support trees at all (e.g. Desert, Mountains)

			auto deposit = OreDepositMap::GetDepositAt(worldX, worldZ, terrainSettings.seed); // NEW
			if (deposit) continue; // NEW — don't place a tree directly on an ore patch


			float aridity = BiomeMap::GetAridity(worldX, worldZ, terrainSettings.seed); // 0 = wet, 1 = dry
			float wetness = 1.0f - aridity;

			// Combine the biome's own baseline density with local wetness — wetter spots within
			// a forest-capable biome get denser trees, drier spots within the same biome get sparser.
			float effectiveDensity = def.treeDensity * wetness;

			float roll = treeJitterNoise.GetNoise(worldX, worldZ); // [-1, 1]
			float threshold = 1.0f - (effectiveDensity * 2.0f);
			if (roll < threshold) continue;

			float jitterX = (rand() / (float)RAND_MAX - 0.5f) * sampleSpacing;
			float jitterZ = (rand() / (float)RAND_MAX - 0.5f) * sampleSpacing;
			float finalX = worldX + jitterX;
			float finalZ = worldZ + jitterZ;

			auto sample = TerrainGenerator::SampleHeight(finalX, finalZ, terrainSettings); // or SampleTerrain if you want ore/depletion-aware height too — height alone doesn't need it

			auto entity = registry.create();
			auto &transform = registry.emplace<TransformComponent>(entity);
			transform.Position = glm::vec3(finalX, sample, finalZ);
			registry.emplace<MeshComponent>(entity, treeMesh);
			registry.emplace<BoundsComponent>(entity, BoundsComponent{ glm::vec3(0.5f, 2.0f, 0.5f) });

			HarvestableComponent harvest;
			harvest.yieldItem = ItemId::Wood;
			harvest.health = 100.0f;
			harvest.maxHealth = 100.0f;
			harvest.yieldPerHit = 1;
			harvest.yieldOnDestroy = 5;
			registry.emplace<HarvestableComponent>(entity, harvest);
			registry.emplace<NameTag>(entity, "Tree");

			GridCoord gridCoord = PlacementGrid::WorldToGrid(transform.Position, terrainSettings.cellSize); // renamed from `coord`
			placementGrid.Register(gridCoord, entity);

			outTreeEntities.push_back(entity);
		}
	}
}

std::shared_ptr<Mesh> WorldGenerator::GetTreeMeshCache(VulkanEngine *engine, MeshRenderer *meshRenderer) {
	if (!s_TreeMeshCache) {
		s_TreeMeshCache = std::make_shared<Mesh>(ModelLoader::LoadModel("Assets/Models/Tree.glb", engine, meshRenderer));
	}
	return s_TreeMeshCache;
}
