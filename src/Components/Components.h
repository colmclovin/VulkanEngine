#pragma once
#include <glm/glm.hpp>
#include <string>
struct SpriteComponent {
    glm::vec2 position = glm::vec2(0.0f); // Screen position
    glm::vec2 size = glm::vec2(100.0f); // Sprite size
    glm::vec4 color = glm::vec4(1.0f); // Color tint
    float rotation = 0.0f; // Rotation in radians
    int layer = 0; // Render layer (higher = front)
};
struct NameTag {
    std::string name;
};