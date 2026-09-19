// AnimationSystem.cpp
#include "AnimationSystem.h"
#include "AnimationComponent.h"
#include "Components.h"
#include "SkinnedMeshComponent.h"
#include "AnimationClip.h"
#include "SkinnedMesh.h"


void AnimationSystem::Update(entt::registry &registry, float deltaTime) {
    auto view = registry.view<SkinnedMeshComponent, AnimationComponent>();
    for (auto entity : view) {
        auto &meshComp = view.get<SkinnedMeshComponent>(entity);
        auto &anim = view.get<AnimationComponent>(entity);

        if (!meshComp.mesh || meshComp.mesh->animations.empty()) continue;
        if (anim.currentClipIndex < 0 || anim.currentClipIndex >= (int)meshComp.mesh->animations.size()) continue;

        const AnimationClip &clip = meshComp.mesh->animations[anim.currentClipIndex];

        float ticksPerSecond = clip.ticksPerSecond;
        anim.playbackTime += deltaTime * ticksPerSecond * anim.playbackSpeed;

        if (anim.playbackTime > clip.duration) {
            if (anim.loop) {
                anim.playbackTime = fmod(anim.playbackTime, clip.duration);
            } else {
                anim.playbackTime = clip.duration; // clamp at the end for one-shot animations
            }
        }
    }
}