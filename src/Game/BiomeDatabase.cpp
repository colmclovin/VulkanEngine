// BiomeDatabase.cpp
#include "BiomeDatabase.h"
#include <stdexcept>

std::unordered_map<BiomeId, BiomeDef> BiomeDatabase::s_Biomes;
std::vector<BiomeId> BiomeDatabase::s_Order;

void BiomeDatabase::Init() {
    s_Biomes[BiomeId::Plains] = {
        "Plains", glm::vec3(0.35f, 0.6f, 0.25f),
        0.0f, 1.0f, 0.0f, 1.0f,
        0.8f, HeightNoiseStyle::Gentle,
        0.2f // ADD — treeDensity
    };
    s_Biomes[BiomeId::Desert] = {
        "Desert", glm::vec3(0.85f, 0.75f, 0.45f),
        0.0f, 0.5f, 0.6f, 1.0f,
        0.6f, HeightNoiseStyle::Gentle,
        0.0f // ADD — no trees
    };
    s_Biomes[BiomeId::Tundra] = {
        "Tundra", glm::vec3(0.85f, 0.87f, 0.9f),
        0.6f, 1.0f, 0.0f, 1.0f,
        1.0f, HeightNoiseStyle::Gentle,
        0.05f // ADD — sparse
    };
    s_Biomes[BiomeId::Hills] = {
        "Hills", glm::vec3(0.4f, 0.55f, 0.3f),
        0.0f, 1.0f, 0.0f, 1.0f,
        5.0f, HeightNoiseStyle::Rugged,
        0.25f // ADD
    };
    s_Biomes[BiomeId::Mountains] = {
        "Mountains", glm::vec3(0.5f, 0.48f, 0.45f),
        0.0f, 1.0f, 0.0f, 1.0f,
        12.0f, HeightNoiseStyle::Rugged,
        0.0f // ADD — no trees
    };
    s_Biomes[BiomeId::Lake] = {
        // ADD — was missing entirely
        "Lake", glm::vec3(0.2f, 0.3f, 0.5f),
        0.0f, 1.0f, 0.0f, 0.5f,
        1.0f, HeightNoiseStyle::Carved,
        0.0f // no trees, though your hard-coded exclusion in ScatterTreesInChunk already guarantees this regardless
    };

    s_Order = { BiomeId::Tundra, BiomeId::Desert, BiomeId::Lake, BiomeId::Plains };
}

const BiomeDef &BiomeDatabase::Get(BiomeId id) {
    auto it = s_Biomes.find(id);
    if (it == s_Biomes.end()) throw std::runtime_error("BiomeDatabase::Get: unregistered biome");
    return it->second;
}

BiomeId BiomeDatabase::DetermineBiome(float latitude, float aridity) {
    for (BiomeId id : s_Order) {
        const BiomeDef &def = s_Biomes[id];
        if (latitude >= def.minLatitude && latitude <= def.maxLatitude &&
            aridity >= def.minAridity && aridity <= def.maxAridity) {
            return id;
        }
    }
    return BiomeId::Plains; // ultimate fallback
}