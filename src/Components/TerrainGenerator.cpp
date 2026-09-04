// TerrainGenerator.cpp
#include "TerrainGenerator.h"
#include "FastNoiseLite.h"   // single-header noise library


glm::vec3 TerrainGenerator::ColorForResource(ItemId resource) {
    switch (resource) {
    case ItemId::CopperOre: return glm::vec3(0.85f, 0.45f, 0.25f);
    case ItemId::IronOre:   return glm::vec3(0.55f, 0.55f, 0.65f);
    default:                return glm::vec3(1.0f);
    }
}


std::shared_ptr<Mesh> TerrainGenerator::GenerateHeightmapTerrain(
    int gridWidth, int gridDepth, float cellSize, float heightScale, float noiseScale, int seed,
    ResourceMap& resourceMap) {

    auto mesh = std::make_shared<Mesh>();

    FastNoiseLite noise;
    noise.SetSeed(seed);
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(noiseScale);

    mesh->Vertices.resize(static_cast<size_t>(gridWidth) * gridDepth);
    for (int z = 0; z < gridDepth; z++) {
        for (int x = 0; x < gridWidth; x++) {
            int index = z * gridWidth + x;

            float worldX = x * cellSize;
            float worldZ = z * cellSize;
            float height = noise.GetNoise(worldX, worldZ) * heightScale;

            Vertex& v = mesh->Vertices[index];
            v.position = glm::vec3(worldX, height, worldZ);
            v.texCoord = glm::vec2(
                static_cast<float>(x) / (gridWidth - 1),
                static_cast<float>(z) / (gridDepth - 1));
            v.normal = glm::vec3(0.0f, 1.0f, 0.0f);

            ResourceCell* cell = resourceMap.GetCellAtWorldPos(worldX, worldZ);
            v.color = (cell) ? ColorForResource(cell->resource) : glm::vec3(1.0f);
        }
    }

    for (int z = 0; z < gridDepth - 1; z++) {
        for (int x = 0; x < gridWidth - 1; x++) {
            int topLeft = z * gridWidth + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * gridWidth + x;
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


float TerrainGenerator::SampleHeight(float worldX, float worldZ, const TerrainSettings& settings) {
    FastNoiseLite noise;
    noise.SetSeed(settings.seed);
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(settings.noiseScale);
    return noise.GetNoise(worldX, worldZ) * settings.heightScale;
}