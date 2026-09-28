// ChunkGpuMetadata.h
#pragma once
#include <glm/glm.hpp>

struct ChunkGpuMetadata {
    glm::vec4 boundsCenterAndRadius;   // xyz = center, w = radius
    uint32_t slot;                      // which TerrainBufferPool slot this chunk's data lives in
    uint32_t isActive;                    // 0 = empty/unloaded slot, 1 = has real chunk data — compute shader skips inactive slots
    uint32_t padding[2];                // std430 alignment padding
};