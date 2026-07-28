#include "AudioEventSystem.h"
#include "AudioEngine.h"
#include <cstdlib>

AudioEventSystem::AudioEventSystem(AudioEngine *engine) : m_Engine(engine) {}

void AudioEventSystem::RegisterSound(AudioEvent event, const std::string &filepath) {
    m_EventSounds[event].push_back(filepath);
}

void AudioEventSystem::Trigger(AudioEvent event, float volume) {
    auto it = m_EventSounds.find(event);
    if (it == m_EventSounds.end() || it->second.empty()) return;

    // Pick a random variant if multiple registered (e.g. 3 footstep sounds for variety)
    const std::string &path = it->second[rand() % it->second.size()];
    m_Engine->PlaySFX(path, volume);
}