// TextureLoader.h
#pragma once
#include <string>
#include <vector>

struct TextureData {
    int width = 0;
    int height = 0;
    std::vector<unsigned char> pixels; // RGBA, 4 bytes per pixel
};

class TextureLoader {
public:
    static TextureData LoadFromFile(const std::string &path); // for standalone image files, if you ever have any
    static TextureData LoadFromMemory(const unsigned char *data, int dataSize); // for embedded glTF/glb textures — the main path
};