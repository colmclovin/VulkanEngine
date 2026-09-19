// Animator.cpp
#include "Animator.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <unordered_map>

glm::vec3 Animator::SamplePosition(const BoneAnimationTrack &track, float time) {
    if (track.positions.empty()) return glm::vec3(0.0f);
    if (track.positions.size() == 1) return track.positions[0].value;

    for (size_t i = 0; i < track.positions.size() - 1; i++) {
        if (time < track.positions[i + 1].time) {
            float t0 = track.positions[i].time;
            float t1 = track.positions[i + 1].time;
            float factor = (time - t0) / (t1 - t0);
            return glm::mix(track.positions[i].value, track.positions[i + 1].value, factor);
        }
    }
    return track.positions.back().value;
}

glm::quat Animator::SampleRotation(const BoneAnimationTrack &track, float time) {
    if (track.rotations.empty()) return glm::quat(1, 0, 0, 0);
    if (track.rotations.size() == 1) return track.rotations[0].value;

    for (size_t i = 0; i < track.rotations.size() - 1; i++) {
        if (time < track.rotations[i + 1].time) {
            float t0 = track.rotations[i].time;
            float t1 = track.rotations[i + 1].time;
            float factor = (time - t0) / (t1 - t0);
            return glm::slerp(track.rotations[i].value, track.rotations[i + 1].value, factor);
        }
    }
    return track.rotations.back().value;
}

glm::vec3 Animator::SampleScale(const BoneAnimationTrack &track, float time) {
    if (track.scales.empty()) return glm::vec3(1.0f);
    if (track.scales.size() == 1) return track.scales[0].value;

    for (size_t i = 0; i < track.scales.size() - 1; i++) {
        if (time < track.scales[i + 1].time) {
            float t0 = track.scales[i].time;
            float t1 = track.scales[i + 1].time;
            float factor = (time - t0) / (t1 - t0);
            return glm::mix(track.scales[i].value, track.scales[i + 1].value, factor);
        }
    }
    return track.scales.back().value;
}

std::vector<glm::mat4> Animator::ComputePose(const Skeleton &skeleton, const AnimationClip &clip, float timeInTicks) {
    std::vector<glm::mat4> localTransforms(skeleton.bones.size(), glm::mat4(1.0f));

    // Build a name -> track lookup for this clip
    std::unordered_map<std::string, const BoneAnimationTrack *> trackByName;
    for (auto &track : clip.tracks) {
        trackByName[track.boneName] = &track;
    }

    for (size_t i = 0; i < skeleton.bones.size(); i++) {
        auto it = trackByName.find(skeleton.bones[i].name);
        if (it == trackByName.end()) continue; // no animation data for this bone — stays identity (rest pose)

        const BoneAnimationTrack &track = *it->second;
        glm::vec3 pos = SamplePosition(track, timeInTicks);
        glm::quat rot = SampleRotation(track, timeInTicks);
        glm::vec3 scale = SampleScale(track, timeInTicks);

        localTransforms[i] = glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(rot) * glm::scale(glm::mat4(1.0f), scale);
    }

    // Walk hierarchy: world = parent.world * local, then apply inverse bind matrix
    std::vector<glm::mat4> worldTransforms(skeleton.bones.size());
    std::vector<glm::mat4> finalMatrices(skeleton.bones.size());

    for (size_t i = 0; i < skeleton.bones.size(); i++) {
        int parent = skeleton.bones[i].parentIndex;
        worldTransforms[i] = (parent >= 0) ? worldTransforms[parent] * localTransforms[i] : localTransforms[i];
        finalMatrices[i] = worldTransforms[i] * skeleton.bones[i].inverseBindMatrix;
    }

    return finalMatrices;
}