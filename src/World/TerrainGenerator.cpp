// TerrainGenerator.cpp
#include "TerrainGenerator.h"
#include "FastNoiseLite.h"   // single-header noise library
#include "../Game/BiomeDatabase.h"
#include "../Game/BiomeMap.h"
#include <glm/gtc/constants.hpp>
#include <iostream>
#include "../Game/OreDatabase.h"
#include "../Game/OreDepositMap.h"
#include "DepletionMap.h"

glm::vec3 TerrainGenerator::ColorForResource(ItemId resource) {
	switch (resource) {
	case ItemId::CopperOre: return glm::vec3(0.85f, 0.45f, 0.25f);
	case ItemId::IronOre:   return glm::vec3(0.55f, 0.55f, 0.65f);
	case ItemId::Coal:      return glm::vec3(0.f, 0.0f, 0.0f);
	default:                return glm::vec3(0.0f, 0.7f, 0.0f);
	}
}


std::shared_ptr<Mesh> TerrainGenerator::GenerateHeightmapTerrain(const TerrainSettings &settings, ResourceMap &resourceMap) {

	auto mesh = std::make_shared<Mesh>();

	FastNoiseLite noise;
	noise.SetSeed(settings.seed);
	noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	noise.SetFrequency(settings.noiseScale);

	mesh->Vertices.resize(static_cast<size_t>(settings.gridWidth) * settings.gridDepth);
	for (int z = 0; z < settings.gridDepth; z++) {
		for (int x = 0; x < settings.gridWidth; x++) {
			int index = z * settings.gridWidth + x;

			float worldX = x * settings.cellSize;
			float worldZ = z * settings.cellSize;
			float height = noise.GetNoise(worldX, worldZ) * settings.heightScale;

			Vertex& v = mesh->Vertices[index];
			v.position = glm::vec3(worldX, height, worldZ);
			v.texCoord = glm::vec2(
				static_cast<float>(x) / (settings.gridWidth - 1),
				static_cast<float>(z) / (settings.gridDepth - 1));
			v.normal = glm::vec3(0.0f, 1.0f, 0.0f);

			ResourceCell* cell = resourceMap.GetCellAtWorldPos(worldX, worldZ);
			v.color = SampleBiomeColor(worldX, worldZ, settings);


		}
	}

	for (int z = 0; z < settings.gridDepth - 1; z++) {
		for (int x = 0; x < settings.gridWidth - 1; x++) {
			int topLeft = z * settings.gridWidth + x;
			int topRight = topLeft + 1;
			int bottomLeft = (z + 1) * settings.gridWidth + x;
			int bottomRight = bottomLeft + 1;

			mesh->Indices.push_back(topLeft);
			mesh->Indices.push_back(bottomLeft);
			mesh->Indices.push_back(topRight);

			mesh->Indices.push_back(topRight);
			mesh->Indices.push_back(bottomLeft);
			mesh->Indices.push_back(bottomRight);
		}
	}

	for (auto& v : mesh->Vertices) v.normal = glm::vec3(0.0f);
	for (size_t i = 0; i < mesh->Indices.size(); i += 3) {
		uint32_t i0 = mesh->Indices[i];
		uint32_t i1 = mesh->Indices[i + 1];
		uint32_t i2 = mesh->Indices[i + 2];

		glm::vec3& p0 = mesh->Vertices[i0].position;
		glm::vec3& p1 = mesh->Vertices[i1].position;
		glm::vec3& p2 = mesh->Vertices[i2].position;

		glm::vec3 faceNormal = glm::normalize(glm::cross(p1 - p0, p2 - p0));
		mesh->Vertices[i0].normal += faceNormal;
		mesh->Vertices[i1].normal += faceNormal;
		mesh->Vertices[i2].normal += faceNormal;
	}
	for (auto& v : mesh->Vertices) v.normal = glm::normalize(v.normal);

	return mesh;
}


// Factor the old per-biome switch statement into its own function, taking biome as a parameter instead of looking it up itself
float TerrainGenerator::SampleRawHeightForBiome(float worldX, float worldZ, BiomeId biome, const TerrainSettings& settings) {
	const BiomeDef& def = BiomeDatabase::Get(biome);

	static thread_local std::unordered_map<int, FastNoiseLite> noiseCache;   // keyed by (seed, style) combined
	int key = settings.seed * 10 + static_cast<int>(def.heightStyle);   // simple combined key

	auto it = noiseCache.find(key);
	if (it == noiseCache.end()) {
		FastNoiseLite noise;
		noise.SetSeed(settings.seed);
		switch (def.heightStyle) {
		case HeightNoiseStyle::Gentle:
			noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
			noise.SetFractalType(FastNoiseLite::FractalType_FBm);
			noise.SetFractalOctaves(2);
			noise.SetFrequency(0.004f);
			break;
		case HeightNoiseStyle::Rugged:
			noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
			noise.SetFractalType(FastNoiseLite::FractalType_FBm);
			noise.SetFractalOctaves(3);
			noise.SetFrequency(0.006f);
			break;
		case HeightNoiseStyle::Ridged:
			noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
			noise.SetFractalType(FastNoiseLite::FractalType_Ridged);
			noise.SetFractalOctaves(3);
			noise.SetFrequency(0.005f);
			break;
		default:
			break;
		}
		it = noiseCache.emplace(key, noise).first;
	}

	FastNoiseLite& cachedNoise = it->second;
	switch (def.heightStyle) {
	case HeightNoiseStyle::Gentle:
	case HeightNoiseStyle::Rugged:
		return cachedNoise.GetNoise(worldX, worldZ) * def.heightScale;
	case HeightNoiseStyle::Ridged:
		return std::abs(cachedNoise.GetNoise(worldX, worldZ)) * def.heightScale;
	case HeightNoiseStyle::Carved:
		return -def.heightScale + cachedNoise.GetNoise(worldX, worldZ) * 0.3f;
	default:
		return 0.0f;
	}
}

std::unordered_map<BiomeId, float> TerrainGenerator::SampleBiomeWeights(float worldX, float worldZ, const TerrainSettings &settings) {
	const float sampleRadius = 20.0f;
	float worldExtentZ = settings.gridDepth * settings.cellSize;

	std::unordered_map<BiomeId, float> weights;

	const int ringCount = 3; // multiple concentric rings instead of one
	const int samplesPerRing = 12; // more samples per ring for smoother angular coverage

	weights[BiomeMap::GetBiomeAt(worldX, worldZ, worldExtentZ, settings.seed)] += 1.0f;

	for (int r = 1; r <= ringCount; r++) {
		float ringRadius = sampleRadius * (static_cast<float>(r) / ringCount);
		float ringWeight = 1.0f - (static_cast<float>(r) / (ringCount + 1)); // inner rings weigh more than outer

		for (int i = 0; i < samplesPerRing; i++) {
			float angle = (float)i / samplesPerRing * glm::two_pi<float>();
			float sx = worldX + cos(angle) * ringRadius;
			float sz = worldZ + sin(angle) * ringRadius;
			weights[BiomeMap::GetBiomeAt(sx, sz, worldExtentZ, settings.seed)] += ringWeight;
		}
	}

	return weights;
}

float TerrainGenerator::SampleHeight(float worldX, float worldZ, const TerrainSettings &settings) {
	auto weights = SampleBiomeWeights(worldX, worldZ, settings);

	float totalWeight = 0.0f;
	float blendedHeight = 0.0f;
	for (auto &[biome, weight] : weights) {
		blendedHeight += SampleRawHeightForBiome(worldX, worldZ, biome, settings) * weight;
		totalWeight += weight;
	}
	return blendedHeight / totalWeight;
}

glm::vec3 TerrainGenerator::SampleBiomeColor(float worldX, float worldZ, const TerrainSettings &settings) {
	float worldExtentZ = settings.gridDepth * settings.cellSize;
	float elevationTrigger = BiomeMap::GetElevationTrigger(worldX, worldZ, settings.seed);

	// Normal climate-based blending (existing ring-sample code) for flat terrain
	auto weights = SampleBiomeWeights(worldX, worldZ, settings);
	float totalWeight = 0.0f;
	glm::vec3 climateColor(0.0f);
	for (auto &[biome, weight] : weights) {
		climateColor += BiomeDatabase::Get(biome).groundColor * weight;
		totalWeight += weight;
	}
	climateColor /= totalWeight;

	if (elevationTrigger > 0.65f) {
		// Mountains: blend toward gray/rock color, but let climate tint show through partially
		glm::vec3 mountainColor = BiomeDatabase::Get(BiomeId::Mountains).groundColor;
		return glm::mix(climateColor, mountainColor, 0.7f); // mostly rock-colored, slight climate tint
	}
	if (elevationTrigger > 0.35f) {
		// Hills: blend more toward climate color, since hills are still vegetated in most climates
		glm::vec3 hillColor = BiomeDatabase::Get(BiomeId::Hills).groundColor;
		return glm::mix(climateColor, hillColor, 0.4f); // mostly climate-colored, slight hill-green tint
	}

	return climateColor;
}
float TerrainGenerator::SmoothStep(float edge0, float edge1, float x) {
	float t = glm::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t); // classic smoothstep — eases in/out, no hard corner
}

TerrainGenerator::TerrainSample TerrainGenerator::SampleTerrain(float worldX, float worldZ, const TerrainSettings &settings, const DepletionMap &depletionMap) {
	float worldExtentZ = settings.gridDepth * settings.cellSize;

	// --- Climate biome, smoothly blended across nearby samples (existing ring technique) ---
	auto climateWeights = SampleBiomeWeights(worldX, worldZ, settings); // unchanged from before
	float totalClimateWeight = 0.0f;
	float climateHeight = 0.0f;
	glm::vec3 climateColor(0.0f);
	for (auto &[biome, weight] : climateWeights) {
		climateHeight += SampleRawHeightForBiome(worldX, worldZ, biome, settings) * weight;
		climateColor += BiomeDatabase::Get(biome).groundColor * weight;
		totalClimateWeight += weight;
	}
	climateHeight /= totalClimateWeight;
	climateColor /= totalClimateWeight;

	// --- Elevation trigger, smoothly weighted instead of hard-thresholded ---
	float elevationTrigger = BiomeMap::GetElevationTrigger(worldX, worldZ, settings.seed);

	// Smooth ramps: hills fade in starting at 0.25, fully "hills" by 0.45;
	// mountains fade in starting at 0.45, fully "mountains" by 0.65.
	// These ranges OVERLAP deliberately, so there's a smooth three-way blend zone, not a sharp edge.
	float hillsWeight = SmoothStep(0.25f, 0.45f, elevationTrigger) * (1.0f - SmoothStep(0.55f, 0.75f, elevationTrigger));
	float mountainsWeight = SmoothStep(0.45f, 0.65f, elevationTrigger);
	float baseWeight = 1.0f - glm::max(hillsWeight, mountainsWeight);
	baseWeight = glm::clamp(baseWeight, 0.0f, 1.0f);

	// Normalize so the three weights always sum to 1
	float totalWeight = baseWeight + hillsWeight + mountainsWeight;
	baseWeight /= totalWeight;
	hillsWeight /= totalWeight;
	mountainsWeight /= totalWeight;

	const BiomeDef &hillsDef = BiomeDatabase::Get(BiomeId::Hills);
	const BiomeDef &mountainsDef = BiomeDatabase::Get(BiomeId::Mountains);

	float hillsHeight = SampleRawHeightForBiome(worldX, worldZ, BiomeId::Hills, settings);
	float mountainsHeight = SampleRawHeightForBiome(worldX, worldZ, BiomeId::Mountains, settings);

	float finalHeight = climateHeight * baseWeight + hillsHeight * hillsWeight + mountainsHeight * mountainsWeight;

	glm::vec3 finalColor = climateColor * baseWeight // ADD THIS BLOCK BACK
						   + glm::mix(climateColor, hillsDef.groundColor, 0.5f) * hillsWeight + glm::mix(climateColor, mountainsDef.groundColor, 0.7f) * mountainsWeight;

	auto deposit = OreDepositMap::GetDepositAt(worldX, worldZ, settings.seed);
	if (deposit) {
		int cellX = static_cast<int>(std::round(worldX));
		int cellZ = static_cast<int>(std::round(worldZ));
		float remaining = depletionMap.GetRemainingFraction(cellX, cellZ);

		if (remaining > 0.0f) {
			glm::vec3 oreColor = OreDatabase::GetOreColor(deposit->item);
			finalColor = glm::mix(finalColor, oreColor, 0.5f * remaining);
		}
	}

	return { finalHeight, finalColor };
}
