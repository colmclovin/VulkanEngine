#pragma once
#include <string>

struct AnimationComponent {
    int currentClipIndex = 0;
    float playbackTime = 0.0f; // in ticks, matching AnimationClip::duration's units
    bool loop = true;
    float playbackSpeed = 1.0f;
};