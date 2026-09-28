#pragma once
#include <cstdint>

struct GpuDrawCommand {   // matches VkDrawIndexedIndirectCommand layout exactly
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t vertexOffset;
    uint32_t firstInstance;
};