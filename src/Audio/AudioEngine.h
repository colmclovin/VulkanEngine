#pragma once
#include <string>
#include <unordered_map>
#include <memory>


struct ma_engine;
struct ma_sound;


class AudioEngine {

public:

	AudioEngine();
    ~AudioEngine();

	void Init();
    void Shutdown();

	void PlayMusic(const std::string &path, bool loop = true, float volume = 1.0f);
    void StopMusic();
    void SetMusicVolume(float volume);

    void PlaySFX(const std::string &path, float volume = 1.0f); // fire-and-forget one-shots

    void SetMasterVolume(float volume);


private:

    ma_engine *m_Engine = nullptr;
    ma_sound *m_MusicSound = nullptr;
    bool m_Initialized = false;

};