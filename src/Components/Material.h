#pragma once
#include <glm/glm.hpp>
#include <string>

struct Material {
    glm::vec4 baseColor = glm::vec4(1.0f);   // diffuse color, used until textures exist
    std::string name;
    // texturePath, roughness, etc. — extend later once a texture pipeline exists
};