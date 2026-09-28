#pragma once
#include "../World/ChunkCoord.h"

struct TreeOriginComponent {
    ChunkCoord chunkCoord;
    int candidateIndex; // this tree's index within its chunk's generated tree-candidate list
};