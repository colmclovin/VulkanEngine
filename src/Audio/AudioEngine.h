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

	void PlayMusic(const std::string &path, bool loop = true, float MusicVolume = 1.0f);
    void StopMusic();
    void SetMusicVolume(float MusicVolume);
    void SetSFXVolume(float SFXVolume);

    void PlaySFX(const std::string &path, float SFXVolume = 1.0f);

    void SetMasterVolume(float MasterVolume);


private:

    ma_engine *m_Engine = nullptr;
    ma_sound *m_MusicSound = nullptr;
    ma_sound *m_SFXGroup = nullptr;
    bool m_Initialized = false;

};