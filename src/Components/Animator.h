// Animator.h
#pragma once
#include "AnimationClip.h"
#include "Skeleton.h"
#include <glm/glm.hpp>
#include <vector>

class Animator {
public:
    static std::vector<glm::mat4> ComputePose(const Skeleton &skeleton, const AnimationClip &clip, float timeInTicks);

private:
    static glm::vec3 SamplePosition(const BoneAnimationTrack &track, float time);
    static glm::quat SampleRotation(const BoneAnimationTrack &track, float time);
    static glm::vec3 SampleScale(const BoneAnimationTrack &track, float time);
};