// Frustum.h
#pragma once
#include <glm/glm.hpp>
#include <array>

struct Frustum {
    std::array<glm::vec4, 6> planes;   // xyz = normal, w = distance; each plane: dot(normal, point) + w >= 0 means inside

    static Frustum FromViewProj(const glm::mat4& viewProj) {
        Frustum f;
        glm::mat4 m = glm::transpose(viewProj);
        f.planes[0] = m[3] + m[0];   // left
        f.planes[1] = m[3] - m[0];   // right
        f.planes[2] = m[3] + m[1];   // bottom
        f.planes[3] = m[3] - m[1];   // top
        f.planes[4] = m[3] + m[2];   // near
        f.planes[5] = m[3] - m[2];   // far

        for (auto& p : f.planes) {
            float len = glm::length(glm::vec3(p));
            if (len > 0.0f) p /= len;
        }
        return f;
    }

    bool ContainsSphere(glm::vec3 center, float radius) const {
        for (auto& p : planes) {
            if (glm::dot(glm::vec3(p), center) + p.w < -radius) {
                return false;   // fully outside this plane
            }
        }
        return true;
    }
};