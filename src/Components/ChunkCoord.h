// ChunkCoord.h
#pragma once
#include <functional>

struct ChunkCoord {
    int x, z;
    bool operator==(const ChunkCoord &o) const { return x == o.x && z == o.z; }
};

struct ChunkCoordHash {
    size_t operator()(const ChunkCoord &c) const {
        return std::hash<int>()(c.x) * 73856093u ^ std::hash<int>()(c.z) * 19349663u;
    }
};