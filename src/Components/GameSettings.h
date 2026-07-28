#pragma once
#include <glm/glm.hpp>
#include <json.hpp>
struct TerrainSettings {
    int gridWidth = 100;
    int gridDepth = 100;
    float cellSize = 1.0f;
    float heightScale = 5.0f;
    float noiseScale = 0.05f;
    int seed = 1337;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TerrainSettings,
        gridWidth, gridDepth, cellSize, heightScale, noiseScale, seed)


};


struct GameSettings {
    float freeFlyMoveSpeed = 2.5f;
    float freeFlyFastMoveSpeed = 7.5f;
    float freeFlySensitivity = 0.1f;
    float isoRotateSpeed = 8.0f;
    float isoDistance = 20.0f;
    float isoPitch = 45.0f;
    float isoZoomSpeed = 1.0f;
    float playerMoveSpeed = 5.0f;
	float playerRunSpeed = 10.0f;
    TerrainSettings terrain;
    bool wireframeMode = false;
    glm::vec4 clearColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
    float masterVolume = 1.0f;
    float musicVolume = 0.5f;


    void SaveToFile(const std::string& path) const;
    static GameSettings LoadFromFile(const std::string& path);
};



