#pragma once
#include <glm/glm.hpp>
#include <optional>
#include "ChunkCoord.h"

class ChunkManager;
class TreeRenderer;
class RemovedTreesMap;

struct TreeHit {
    ChunkCoord chunk;
    int gridIndex;       // the tree's stable index within its chunk (used by RemovedTreesMap/TreeHealthMap)
    glm::vec3 basePos;
    float t;             // distance along the ray
};

// Trees are GPU instances, not entities, so picking them means reading back each chunk's instance list.
class TreeRaycast {
public:
    // Searches the 3x3 chunk neighborhood around nearPoint for the closest standing tree the ray hits.
    static std::optional<TreeHit> FindTreeAlongRay(glm::vec3 rayOrigin, glm::vec3 rayDir, glm::vec3 nearPoint, float maxT,
                                                   const ChunkManager &chunkManager, const TreeRenderer &treeRenderer,
                                                   const RemovedTreesMap &removedTrees);
};
