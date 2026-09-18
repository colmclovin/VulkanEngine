#pragma once
#include <glm/glm.hpp>
#include <string>
#include <memory>
#include "Texture.h"
#include "TextureLoader.h"

struct Material {
    glm::vec4 baseColor = glm::vec4(1.0f);   // diffuse color, used until textures exist
    std::string name;
    std::shared_ptr<Texture> texture; // texturePath, roughness, etc. — extend later once a texture pipeline exists
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE; // NEW — set once, after texture upload
};