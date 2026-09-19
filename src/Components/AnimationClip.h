#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

struct PositionKey {
    float time;
    glm::vec3 value;
};
struct RotationKey {
    float time;
    glm::quat value;
};
struct ScaleKey {
    float time;
    glm::vec3 value;
};

struct BoneAnimationTrack {
    std::string boneName;
    std::vector<PositionKey> positions;
    std::vector<RotationKey> rotations;
    std::vector<ScaleKey> scales;
};

struct AnimationClip {
    std::string name;
    float duration = 0.0f; // in ticks
    float ticksPerSecond = 25.0f;
    std::vector<BoneAnimationTrack> tracks;
};