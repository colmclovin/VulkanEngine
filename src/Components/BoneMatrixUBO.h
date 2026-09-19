#pragma once
#include <glm/glm.hpp>

constexpr int MAX_BONES = 64; // generous ceiling; your test rig only needs 2

struct BoneMatrixUBO {
    glm::mat4 boneMatrices[MAX_BONES];
};