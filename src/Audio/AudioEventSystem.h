#pragma once
#include "AudioEvent.h"
#include <string>
#include <unordered_map>
#include <vector>

class AudioEngine;

class AudioEventSystem {
public:
    AudioEventSystem(AudioEngine *engine);

    void RegisterSound(AudioEvent event, const std::string &filepath);
    void Trigger(AudioEvent event, float volume = 1.0f);

private:
    AudioEngine *m_Engine = nullptr;
    std::unordered_map<AudioEvent, std::vector<std::string>> m_EventSounds; // supports multiple variants per event
};