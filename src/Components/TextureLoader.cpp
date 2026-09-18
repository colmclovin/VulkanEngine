// TextureLoader.cpp
#define STB_IMAGE_IMPLEMENTATION
#include "TextureLoader.h"
#include <stb/stb_image.h>
#include <stdexcept>

TextureData TextureLoader::LoadFromFile(const std::string &path) {
    TextureData data;
    int channels;
    unsigned char *pixels = stbi_load(path.c_str(), &data.width, &data.height, &channels, STBI_rgb_alpha);
    if (!pixels) throw std::runtime_error("Failed to load texture: " + path);

    size_t byteCount = static_cast<size_t>(data.width) * data.height * 4;
    data.pixels.assign(pixels, pixels + byteCount);
    stbi_image_free(pixels);
    return data;
}

TextureData TextureLoader::LoadFromMemory(const unsigned char *fileData, int dataSize) {
    TextureData data;
    int channels;
    unsigned char *pixels = stbi_load_from_memory(fileData, dataSize, &data.width, &data.height, &channels, STBI_rgb_alpha);
    if (!pixels) throw std::runtime_error("Failed to decode embedded texture");

    size_t byteCount = static_cast<size_t>(data.width) * data.height * 4;
    data.pixels.assign(pixels, pixels + byteCount);
    stbi_image_free(pixels);
    return data;
}