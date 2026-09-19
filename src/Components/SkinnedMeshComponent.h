#pragma once
#include <memory>

class SkinnedMesh;

struct SkinnedMeshComponent {
    std::shared_ptr<SkinnedMesh> mesh;
    // later, once we get to Stage D: current animation clip index, playback time, etc.
};