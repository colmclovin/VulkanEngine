#include "PlayerAnimationSystem.h"
#include "AnimationComponent.h"
#include "Components.h"
#include "SkinnedMeshComponent.h"
#include <glm/glm.hpp>
#include "SkinnedMesh.h"

void PlayerAnimationSystem::Update(entt::registry &registry, entt::entity player, glm::vec3 velocity) {
    if (!registry.any_of<SkinnedMeshComponent, AnimationComponent>(player)) return;

    auto &meshComp = registry.get<SkinnedMeshComponent>(player);
    auto &anim = registry.get<AnimationComponent>(player);
    if (!meshComp.mesh) return;

    float speed = glm::length(velocity);

    std::string desiredClip;
    if (speed < 0.1f)
        desiredClip = "Idle";
    else if (speed < 5.0f)
        desiredClip = "Walk";
    else
        desiredClip = "Run";

    int desiredIndex = meshComp.mesh->FindClipIndex(desiredClip);
    if (desiredIndex >= 0 && desiredIndex != anim.currentClipIndex) {
        anim.currentClipIndex = desiredIndex;
        anim.playbackTime = 0.0f; // restart cleanly on the new clip
    }
}