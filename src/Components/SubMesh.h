#pragma once
#include <cstdint>

struct SubMesh {
    uint32_t indexOffset = 0;   // where this submesh's indices start, in the shared index buffer
    uint32_t indexCount = 0;    // how many indices belong to this submesh
    int materialIndex = -1;     // index into Mesh::Materials
};