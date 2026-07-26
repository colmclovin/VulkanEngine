// GameSettings.cpp
#include "GameSettings.h"
#include <fstream>
#include <iostream>

namespace glm {
    void to_json(nlohmann::json& j, const glm::vec4& v) {
        j = nlohmann::json{ v.x, v.y, v.z, v.w };
    }
    void from_json(const nlohmann::json& j, glm::vec4& v) {
        v.x = j.at(0).get<float>();
        v.y = j.at(1).get<float>();
        v.z = j.at(2).get<float>();
        v.w = j.at(3).get<float>();
    }
}

void GameSettings::SaveToFile(const std::string& path) const {
    nlohmann::json j;
    j["freeFlyMoveSpeed"] = freeFlyMoveSpeed;
	j["freeFlyFastMoveSpeed"] = freeFlyFastMoveSpeed;
    j["freeFlySensitivity"] = freeFlySensitivity;
    j["isoRotateSpeed"] = isoRotateSpeed;
    j["isoDistance"] = isoDistance;
    j["isoPitch"] = isoPitch;
    j["playerMoveSpeed"] = playerMoveSpeed;
	j["playerRunSpeed"] = playerRunSpeed;
    j["terrain"] = terrain;   // uses the macro-generated conversion
    j["wireframeMode"] = wireframeMode;
    j["clearColor"] = clearColor;

    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open settings file for writing: " << path << std::endl;
        return;
    }
    file << j.dump(4);   // pretty-print with 4-space indent
    std::cout << "Settings saved to " << path << std::endl;
}

GameSettings GameSettings::LoadFromFile(const std::string& path) {
    GameSettings settings;   // defaults

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "No settings file found at " << path << ", creating one with defaults." << std::endl;
        settings.SaveToFile(path);   // NEW — write defaults out so the file exists next run
        return settings;
    }

    try {
        nlohmann::json j;
        file >> j;

        settings.freeFlyMoveSpeed = j.value("freeFlyMoveSpeed", settings.freeFlyMoveSpeed);
        settings.freeFlySensitivity = j.value("freeFlySensitivity", settings.freeFlySensitivity);
        settings.isoRotateSpeed = j.value("isoRotateSpeed", settings.isoRotateSpeed);
        settings.isoDistance = j.value("isoDistance", settings.isoDistance);
        settings.isoPitch = j.value("isoPitch", settings.isoPitch);
        settings.playerMoveSpeed = j.value("playerMoveSpeed", settings.playerMoveSpeed);
        if (j.contains("terrain")) settings.terrain = j.at("terrain").get<TerrainSettings>();
        settings.wireframeMode = j.value("wireframeMode", settings.wireframeMode);
        if (j.contains("clearColor")) settings.clearColor = j.at("clearColor").get<glm::vec4>();

        std::cout << "Settings loaded from " << path << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to parse settings file: " << e.what() << ". Using defaults." << std::endl;
        return GameSettings{};
    }

    return settings;
}