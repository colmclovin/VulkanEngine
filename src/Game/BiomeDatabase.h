// BiomeDatabase.h
#pragma once
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

enum class BiomeId {
    Plains,
    Desert,
    Tundra,
    Hills,
    Mountains,
    Lake,
};

enum class HeightNoiseStyle {
    Gentle, // plains — low amplitude, smooth rolling
    Rugged, // hills — moderate amplitude
    Ridged, // mountains — sharp, high amplitude ridged noise
    Carved, // lake — flattened, depressed
};

struct BiomeDef {
    std::string name;
    glm::vec3 groundColor; // placeholder until real textures exist
    float minLatitude; // 0 = equator, 1 = pole
    float maxLatitude;
    float minAridity; // 0 = wet, 1 = dry
    float maxAridity;
    float heightScale;
    HeightNoiseStyle heightStyle;
    float treeDensity = 0.0f; // ADD — 0 = no trees supported at all, higher = denser baseline forest
};

class BiomeDatabase {
public:
    static void Init();
    static const BiomeDef &Get(BiomeId id);
    static BiomeId DetermineBiome(float latitude, float aridity); // the lookup logic

private:
    static std::unordered_map<BiomeId, BiomeDef> s_Biomes;
    static std::vector<BiomeId> s_Order; // evaluation order for DetermineBiome
};