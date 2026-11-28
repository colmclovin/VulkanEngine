#pragma once
#include <glm/glm.hpp>

namespace GpuNoiseMatch {
    float HashSeed(float seed);
    float SimplexNoise2D(glm::vec2 v);
    float Fbm(glm::vec2 pos, int octaves, float frequency, float lacunarity, float gain);
    float RidgedNoise(glm::vec2 pos, int octaves, float frequency, float lacunarity, float gain);

    float GetAridity(float worldX, float worldZ, float seed);
    float GetElevationTrigger(float worldX, float worldZ, float seed);
    float GetLatitude(float worldZ);
    int DetermineBiome(float worldX, float worldZ, float seed);   // returns BIOME_* int constant

    float GetBiomeHeightScale(int biomeId);
    float SampleRawHeightForBiome(float worldX, float worldZ, int biomeId, float seed);
    void SampleBiomeWeights(float worldX, float worldZ, float seed, float weights[6]);
    // Exact noise height at a point. The rendered terrain only matches this AT mesh vertices;
    // between them the mesh is flat triangles, so use SampleSurfaceHeight to stand things on the ground.
    float SampleHeight(float worldX, float worldZ, float seed);
    float SampleMeshHeight(float worldX, float worldZ, float seed, float cellSize);
    float SampleHeightFast(float worldX, float worldZ, float seed);

    // Spacing between terrain mesh vertices: ChunkManager::CHUNK_WORLD_SIZE / (CHUNK_VERTEX_RESOLUTION - 1).
    // ChunkManager static_asserts that these stay in sync.
    constexpr float TERRAIN_MESH_CELL_SIZE = 32.0f / 15.0f;

    // Height of the rendered terrain surface (interpolated across the mesh triangle) — use for placing units/players.
    inline float SampleSurfaceHeight(float worldX, float worldZ, float seed) {
        return SampleMeshHeight(worldX, worldZ, seed, TERRAIN_MESH_CELL_SIZE);
    }
}

// Match biome.glsl's constants exactly
constexpr int BIOME_PLAINS = 0;
constexpr int BIOME_DESERT = 1;
constexpr int BIOME_TUNDRA = 2;
constexpr int BIOME_HILLS = 3;
constexpr int BIOME_MOUNTAINS = 4;
constexpr int BIOME_LAKE = 5;