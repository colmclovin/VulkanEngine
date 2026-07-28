#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "AudioEngine.h"
#include <iostream>

AudioEngine::AudioEngine() {}
AudioEngine::~AudioEngine() {
    Shutdown();
}


void AudioEngine::Init() {
    m_Engine = new ma_engine();
    ma_result result = ma_engine_init(nullptr, m_Engine);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to initialize audio engine" << std::endl;
        delete m_Engine;
        m_Engine = nullptr;
        return;
    }
    m_Initialized = true;
    std::cout << "Audio engine initialized" << std::endl;
}

void AudioEngine::Shutdown() {
    if (!m_Initialized) return;
    if (m_MusicSound) {
        ma_sound_uninit(m_MusicSound);
        delete m_MusicSound;
        m_MusicSound = nullptr;
    }
    if (m_Engine) {
        ma_engine_uninit(m_Engine);
        delete m_Engine;
        m_Engine = nullptr;
    }
    m_Initialized = false;
    std::cout << "Audio engine shut down" << std::endl;
}

void AudioEngine::PlayMusic(const std::string &path, bool loop, float volume) {
    if (!m_Initialized) return;

    if (m_MusicSound) {
        ma_sound_uninit(m_MusicSound);
        delete m_MusicSound;
        m_MusicSound = nullptr;
    }

    m_MusicSound = new ma_sound();
    ma_result result = ma_sound_init_from_file(m_Engine, path.c_str(), 0, nullptr, nullptr, m_MusicSound);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to load music: " << path << std::endl;
        delete m_MusicSound;
        m_MusicSound = nullptr;
        return;
    }

    ma_sound_set_looping(m_MusicSound, loop ? MA_TRUE : MA_FALSE);
    ma_sound_set_volume(m_MusicSound, volume);
    ma_sound_start(m_MusicSound);
}

void AudioEngine::StopMusic() {
    if (m_MusicSound) {
        ma_sound_stop(m_MusicSound);
    }
}

void AudioEngine::SetMusicVolume(float volume) {
    if (m_MusicSound) {
        ma_sound_set_volume(m_MusicSound, volume);
    }
}

void AudioEngine::PlaySFX(const std::string &path, float volume) {
    if (!m_Initialized) return;
    // ma_engine_play_sound is fire-and-forget: engine manages the sound's lifetime internally
    ma_engine_play_sound(m_Engine, path.c_str(), nullptr);
    // Note: per-call volume needs ma_sound_group or a manual ma_sound if you need it per-SFX;
    // start simple with engine-wide SFX volume for now (see SetMasterVolume)
}

void AudioEngine::SetMasterVolume(float volume) {
    if (m_Initialized) {
        ma_engine_set_volume(m_Engine, volume);
    }
}