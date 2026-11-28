#pragma once
#include <glm/glm.hpp>
#include <algorithm>
#include <limits>
#include <utility>

namespace RayMath {

// Slab test. On hit, outT is the distance along rayDir to the entry point (0 if the origin is inside).
inline bool RayIntersectsAABB(glm::vec3 rayOrigin, glm::vec3 rayDir, glm::vec3 boxMin, glm::vec3 boxMax, float &outT) {
    float tMin = 0.0f, tMax = std::numeric_limits<float>::max();
    for (int axis = 0; axis < 3; axis++) {
        float invD = 1.0f / rayDir[axis];
        float t0 = (boxMin[axis] - rayOrigin[axis]) * invD;
        float t1 = (boxMax[axis] - rayOrigin[axis]) * invD;
        if (invD < 0.0f) std::swap(t0, t1);
        tMin = std::max(tMin, t0);
        tMax = std::min(tMax, t1);
        if (tMax <= tMin) return false;
    }
    outT = tMin;
    return true;
}

// Box whose pivot is at the bottom-center (how entities are positioned in this game).
inline bool RayIntersectsBaseBox(glm::vec3 rayOrigin, glm::vec3 rayDir, glm::vec3 basePos, glm::vec3 halfExtents, float &outT) {
    glm::vec3 center = basePos + glm::vec3(0.0f, halfExtents.y, 0.0f);
    return RayIntersectsAABB(rayOrigin, rayDir, center - halfExtents, center + halfExtents, outT);
}

} // namespace RayMath
