#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>       // glm::quat, glm::mat4_cast
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::scale


struct TransformComponent {
    glm::vec3 Position{ 0.0f };
    glm::quat Rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 Scale{ 1.0f };

    glm::mat4 GetMatrix() const {
        return glm::translate(glm::mat4(1.0f), Position) * glm::mat4_cast(Rotation) * glm::scale(glm::mat4(1.0f), Scale);
    }
};