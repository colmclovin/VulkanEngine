#include "SaveManager.h"
#include "../Components/Components.h"
#include "../Components/GameSettings.h"
#include "../Components/InventoryComponent.h"
#include "../Components/ResourceMap.h"
#include "../Components/PlacementGrid.h"
#include <fstream>
#include <iostream>
#include <JSON/json.hpp>
#include "../Game/PlaceableDatabase.h"
#include "../Components/ModelLoader.h"
#include "../Components/Mesh.h"
#include "../Components/TerrainGenerator.h"

using json = nlohmann::json;

void SaveManager::SaveGame(const std::string &path, entt::registry &registry, entt::entity player,
                           ResourceMap &resourceMap, TechState &techState, const GameSettings &settings, VulkanEngine *engine, MeshRenderer *meshRenderer) {
    json j;
    j["seed"] = settings.terrain.seed;

    j["player"]["transform"] = registry.get<TransformComponent>(player);
    j["player"]["inventory"] = registry.get<InventoryComponent>(player);


    j["techState"]["techPoints"] = techState.techPoints;
    j["techState"]["unlocked"] = techState.GetUnlockedList();


    json cellsJson = json::array();
    for (int z = 0; z < resourceMap.getGridDepth(); z++) {
        for (int x = 0; x < resourceMap.getGridWidth(); x++) {
            ResourceCell &cell = resourceMap.GetCell(x, z);
            if (cell.resource != ItemId::None) {
                cellsJson.push_back({ { "x", x }, { "z", z }, { "resource", cell.resource }, { "amount", cell.amount } });
            }
        }
    }
    j["resourceMapCells"] = cellsJson;



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
        }else if (registry.any_of<HarvestableComponent>(entity)) {
        entityJson["type"] = "Tree";
        entityJson["data"] = registry.get<HarvestableComponent>(entity);
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
                           ResourceMap &resourceMap, PlacementGrid &placementGrid, TechState &techState, GameSettings &settings, VulkanEngine *engine, MeshRenderer *meshRenderer) {
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


    resourceMap.Generate(settings.terrain.gridWidth, settings.terrain.gridDepth, settings.terrain.cellSize, settings.terrain.seed);

    // Clear whatever Generate() produced from fresh noise — saved state must fully override it,
    // since deposits get mined down over time and no longer match what regeneration would produce.
    for (int z = 0; z < resourceMap.getGridDepth(); z++) {
        for (int x = 0; x < resourceMap.getGridWidth(); x++) {
            resourceMap.GetCell(x, z).resource = ItemId::None;
            resourceMap.GetCell(x, z).amount = 0.0f;
        }
    }

    for (auto &cellJson : j["resourceMapCells"]) {
        int x = cellJson["x"];
        int z = cellJson["z"];
        ResourceCell &cell = resourceMap.GetCell(x, z);
        cell.resource = cellJson["resource"].get<ItemId>();
        cell.amount = cellJson["amount"];
    }

    auto terrainMesh = TerrainGenerator::GenerateHeightmapTerrain(
            settings.terrain.gridWidth, settings.terrain.gridDepth,
            settings.terrain.cellSize, settings.terrain.heightScale,
            settings.terrain.noiseScale, settings.terrain.seed,
            resourceMap); // uses the ALREADY-restored resource map for correct color tinting

    auto terrainEntity = registry.create();
    registry.emplace<TransformComponent>(terrainEntity);
    registry.emplace<MeshComponent>(terrainEntity, terrainMesh);
    registry.emplace<NameTag>(terrainEntity, "Terrain");



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
        } else if (type == "Tree") { // FIXED
            registry.emplace<HarvestableComponent>(entity, entityJson["data"].get<HarvestableComponent>());
            auto treeMesh = std::make_shared<Mesh>(ModelLoader::LoadModel("Assets/Models/Tree.glb", engine, meshRenderer));
            registry.emplace<MeshComponent>(entity, treeMesh);
        } else {
            registry.destroy(entity); // unknown type — skip
            continue;
        }

        if (entityJson.contains("powerConsumer")) {
            registry.emplace<PowerConsumerComponent>(entity, entityJson["powerConsumer"].get<PowerConsumerComponent>());
        }

        // Re-register in the placement grid using the item's real footprint
        const PlaceableDef *def = PlaceableDatabase::TryGet(ItemDatabase::FromString(type));
        if (def) {
            registry.emplace<BoundsComponent>(entity, BoundsComponent{ def->footprintHalfExtents });
            auto coveredCells = PlacementGrid::GetCoveredCells(pos, def->footprintHalfExtents, settings.terrain.cellSize);
            placementGrid.RegisterArea(coveredCells, entity);
        } else if (type == "Tree") {
            registry.emplace<BoundsComponent>(entity, BoundsComponent{ glm::vec3(0.5f, 2.0f, 0.5f) }); // match WorldGenerator::SpawnTree's original bounds
            // trees don't occupy the placement grid — they're not player-placed machines, so no PlacementGrid registration needed
        }
    }



outPlayer = registry.create();
    registry.emplace<TransformComponent>(outPlayer, j["player"]["transform"].get<TransformComponent>());
    registry.emplace<PlayerComponent>(outPlayer);
    registry.emplace<InventoryComponent>(outPlayer, j["player"]["inventory"].get<InventoryComponent>());

    auto playerMesh = std::make_shared<Mesh>(ModelLoader::LoadModel("Assets/Models/Test1.glb", engine, meshRenderer));
    registry.emplace<MeshComponent>(outPlayer, playerMesh);
    registry.emplace<NameTag>(outPlayer, "Player");


    std::cout << "Game loaded from " << path << std::endl;
    return true;
}