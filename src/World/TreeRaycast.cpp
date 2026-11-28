#include "TreeRaycast.h"
#include "RemovedTreesMap.h"
#include "../Game/ChunkManager.h"
#include "../Renderer/TreeRenderer.h"
#include "../Utils/RayMath.h"

namespace {
    // Approximate trunk+canopy box around each tree's base position
    constexpr glm::vec3 TREE_BOX_MIN{ -0.6f, 0.0f, -0.6f };
    constexpr glm::vec3 TREE_BOX_MAX{ 0.6f, 3.0f, 0.6f };
}

std::optional<TreeHit> TreeRaycast::FindTreeAlongRay(glm::vec3 rayOrigin, glm::vec3 rayDir, glm::vec3 nearPoint, float maxT,
                                                     const ChunkManager &chunkManager, const TreeRenderer &treeRenderer,
                                                     const RemovedTreesMap &removedTrees) {
    std::optional<TreeHit> best;
    float bestT = maxT;
    ChunkCoord center = ChunkManager::WorldToChunkCoord(nearPoint);

    // A tree near a chunk edge can overlap the neighbor, so check all 9 surrounding chunks
    for (int dz = -1; dz <= 1; dz++) {
        for (int dx = -1; dx <= 1; dx++) {
            ChunkCoord coord{ center.x + dx, center.z + dz };
            auto slot = chunkManager.GetTreeSlotForChunk(coord);
            if (!slot) continue;

            for (const auto &tree : treeRenderer.ReadBackChunkInstances(*slot)) {
                if (removedTrees.IsRemoved(coord, tree.gridIndex)) continue;
                float t;
                if (RayMath::RayIntersectsAABB(rayOrigin, rayDir, tree.position + TREE_BOX_MIN, tree.position + TREE_BOX_MAX, t) &&
                    t < bestT) {
                    bestT = t;
                    best = TreeHit{ coord, tree.gridIndex, tree.position, t };
                }
            }
        }
    }
    return best;
}
