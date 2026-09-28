#include "SaveManager.h"
#include "../Components/Components.h"
#include "../Utils/GameSettings.h"
#include "../Components/InventoryComponent.h"
#include "../World/ResourceMap.h"
#include "../World/PlacementGrid.h"
#include <fstream>
#include <iostream>
#include <JSON/json.hpp>
#include "../Game/PlaceableDatabase.h"
#include "../Rendering/ModelLoader.h"
#include "../Rendering/Mesh.h"
#include "../World/TerrainGenerator.h"
#include "../Components/Components.h"

using json = nlohmann::json;

void SaveManager::SaveGame(const std::string &path, entt::registry &registry, entt::entity player,
                           ResourceMap &resourceMap, TechState &techState, const GameSettings &settings, VulkanEngine *engine, MeshRenderer *meshRenderer, DepletionMap &depletionMap, RemovedTreesMap &removedTreesMap) {
    json j;
    j["seed"] = settings.terrain.seed;

    j["player"]["transform"] = registry.get<TransformComponent>(player);
    j["player"]["inventory"] = registry.get<InventoryComponent>(player);


    j["techState"]["techPoints"] = techState.techPoints;
    j["techState"]["unlocked"] = techState.GetUnlockedList();

json depletionJson = json::array();
    for (auto &[coord, remainingFraction] : depletionMap.GetAll()) {
        depletionJson.push_back({ { "x", coord.x }, { "z", coord.z }, { "remaining", remainingFraction } });
    }
    j["depletionMap"] = depletionJson;



    json removedTreesJson = json::array();
    for (auto &[coord, indices] : removedTreesMap.GetAll()) {
        json indicesJson = json::array();
        for (int idx : indices)
            indicesJson.push_back(idx);
        removedTreesJson.push_back({ { "chunkX", coord.x }, { "chunkZ", coord.z }, { "indices", indicesJson } });
    }
    j["removedTreesMap"] = removedTreesJson;


    json entitiesJson = json::array();
    auto view = registry.view<TransformComponent>();
    for (auto entity : view) {
        if (entity == player) continue;

        json entityJson;
        entityJson["transform"] = view.get<TransformComponent>(entity);

        if (registry.any_of<MinerComponent>(entity)) {
            entityJson["type"] = "Miner";
            entityJson["data"] = registry.get<MinerComponent>(entity);
        } else if (registry.any_of<FurnaceComponent>(entity)) {
            entityJson["type"] = "Furnace";
            entityJson["data"] = registry.get<FurnaceComponent>(entity);
            if (registry.any_of<MachineInventoryComponent>(entity))
                entityJson["inventory"] = registry.get<MachineInventoryComponent>(entity);
            if (registry.any_of<PointLightComponent>(entity)) // NEW
                entityJson["pointLight"] = registry.get<PointLightComponent>(entity);
        } else if (registry.any_of<AssemblerComponent>(entity)) {
            entityJson["type"] = "Assembler";
            entityJson["data"] = registry.get<AssemblerComponent>(entity);
            if (registry.any_of<MachineInventoryComponent>(entity))
                entityJson["inventory"] = registry.get<MachineInventoryComponent>(entity);
        } else if (registry.any_of<BeltComponent>(entity)) {
            entityJson["type"] = "Belt";
            entityJson["data"] = registry.get<BeltComponent>(entity);
        } else if (registry.any_of<InserterComponent>(entity)) {
            entityJson["type"] = "Inserter";
            entityJson["data"] = registry.get<InserterComponent>(entity);
        } else if (registry.any_of<PowerGeneratorComponent>(entity)) {
            entityJson["type"] = "CoalGenerator";
            entityJson["data"] = registry.get<PowerGeneratorComponent>(entity);
            if (registry.any_of<MachineInventoryComponent>(entity))
                entityJson["inventory"] = registry.get<MachineInventoryComponent>(entity);
        
        } else if (registry.any_of<PowerPoleComponent>(entity)) {
            entityJson["type"] = "PowerPole";
            entityJson["data"] = registry.get<PowerPoleComponent>(entity);
        } else {
            continue; // terrain, trees, pickups — not saved yet
        }

        if (registry.any_of<PowerConsumerComponent>(entity)) {
            entityJson["powerConsumer"] = registry.get<PowerConsumerComponent>(entity);
        }

        entitiesJson.push_back(entityJson);
    }
    j["entities"] = entitiesJson;




    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open save file for writing: " << path << std::endl;
        return;
    }
    file << j.dump(4);
    std::cout << "Game saved to " << path << std::endl;
}

bool SaveManager::LoadGame(const std::string &path, entt::registry &registry, entt::entity &outPlayer,
                           ResourceMap &resourceMap, PlacementGrid &placementGrid, TechState &techState, GameSettings &settings, VulkanEngine *engine, MeshRenderer *meshRenderer, DepletionMap &depletionMap, RemovedTreesMap &removedTreesMap) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Save file not found: " << path << std::endl;
        return false;
    }

    json j;
    file >> j;

    settings.terrain.seed = j["seed"];

    techState.techPoints = j["techState"]["techPoints"];
    for (auto &techJson : j["techState"]["unlocked"]) {
        techState.ForceUnlock(techJson.get<TechId>());
    }

       for (auto &entry : j["depletionMap"]) {
        int x = entry["x"];
        int z = entry["z"];
        float remaining = entry["remaining"];
        depletionMap.SetRemainingFraction(x, z, remaining); // small helper, see note below
    }

    for (auto &entry : j["removedTreesMap"]) {
        ChunkCoord coord{ entry["chunkX"], entry["chunkZ"] };
        for (auto &idx : entry["indices"]) {
            removedTreesMap.MarkRemoved(coord, idx.get<int>());
        }
    }


    for (auto &entityJson : j["entities"]) {
        std::string type = entityJson["type"];
        auto entity = registry.create();

        registry.emplace<TransformComponent>(entity, entityJson["transform"].get<TransformComponent>());

        glm::vec3 pos = entityJson["transform"]["Position"].get<glm::vec3>();

        if (type == "Miner") {
            registry.emplace<MinerComponent>(entity, entityJson["data"].get<MinerComponent>());
            auto mesh = ItemDatabase::GetWorldMesh(ItemId::Miner, engine, meshRenderer);
            if (mesh) registry.emplace<MeshComponent>(entity, mesh);
        } else if (type == "Furnace") {
            registry.emplace<FurnaceComponent>(entity, entityJson["data"].get<FurnaceComponent>());
            if (entityJson.contains("inventory"))
                registry.emplace<MachineInventoryComponent>(entity, entityJson["inventory"].get<MachineInventoryComponent>());
            if (entityJson.contains("pointLight")) // NEW
                registry.emplace<PointLightComponent>(entity, entityJson["pointLight"].get<PointLightComponent>());
            auto mesh = ItemDatabase::GetWorldMesh(ItemId::Furnace, engine, meshRenderer);
            if (mesh) registry.emplace<MeshComponent>(entity, mesh);
        } else if (type == "Assembler") {
            registry.emplace<AssemblerComponent>(entity, entityJson["data"].get<AssemblerComponent>());
            if (entityJson.contains("inventory"))
                registry.emplace<MachineInventoryComponent>(entity, entityJson["inventory"].get<MachineInventoryComponent>());
            auto mesh = ItemDatabase::GetWorldMesh(ItemId::Assembler, engine, meshRenderer);
            if (mesh) registry.emplace<MeshComponent>(entity, mesh);
        } else if (type == "Belt") {
            registry.emplace<BeltComponent>(entity, entityJson["data"].get<BeltComponent>());
            auto mesh = ItemDatabase::GetWorldMesh(ItemId::Belt, engine, meshRenderer);
            if (mesh) registry.emplace<MeshComponent>(entity, mesh);
        } else if (type == "Inserter") {
            registry.emplace<InserterComponent>(entity, entityJson["data"].get<InserterComponent>());
            auto mesh = ItemDatabase::GetWorldMesh(ItemId::Inserter, engine, meshRenderer);
            if (mesh) registry.emplace<MeshComponent>(entity, mesh);
        } else if (type == "CoalGenerator") {
            registry.emplace<PowerGeneratorComponent>(entity, entityJson["data"].get<PowerGeneratorComponent>());
            if (entityJson.contains("inventory"))
                registry.emplace<MachineInventoryComponent>(entity, entityJson["inventory"].get<MachineInventoryComponent>());
            auto mesh = ItemDatabase::GetWorldMesh(ItemId::CoalGenerator, engine, meshRenderer);
            if (mesh) registry.emplace<MeshComponent>(entity, mesh);
        } else if (type == "PowerPole") {
            registry.emplace<PowerPoleComponent>(entity, entityJson["data"].get<PowerPoleComponent>());
            auto mesh = ItemDatabase::GetWorldMesh(ItemId::PowerPole, engine, meshRenderer);
            if (mesh) registry.emplace<MeshComponent>(entity, mesh);
        
        } else {
            registry.destroy(entity); // unknown type — skip
            continue;
        }

          if (entityJson.contains("powerConsumer")) {
            registry.emplace<PowerConsumerComponent>(entity, entityJson["powerConsumer"].get<PowerConsumerComponent>());
        }

        const PlaceableDef *def = PlaceableDatabase::TryGet(ItemDatabase::FromString(type));
        if (def) {
            registry.emplace<BoundsComponent>(entity, BoundsComponent{ def->footprintHalfExtents });
            auto coveredCells = PlacementGrid::GetCoveredCells(pos, def->footprintHalfExtents, settings.terrain.cellSize);
            placementGrid.RegisterArea(coveredCells, entity);
        }
    }



outPlayer = registry.create();
    registry.emplace<TransformComponent>(outPlayer, j["player"]["transform"].get<TransformComponent>());
    registry.emplace<PlayerComponent>(outPlayer);
    registry.emplace<InventoryComponent>(outPlayer, j["player"]["inventory"].get<InventoryComponent>());

    auto playerMesh = std::make_shared<SkinnedMesh>(
            ModelLoader::LoadSkinnedModel("Assets/Models/Test1.glb", engine, meshRenderer));
    registry.emplace<SkinnedMeshComponent>(outPlayer, playerMesh); // CHANGED — was MeshComponent/Mesh
    registry.emplace<AnimationComponent>(outPlayer);
    registry.emplace<NameTag>(outPlayer, "Player");


    std::cout << "Game loaded from " << path << std::endl;
    return true;
}