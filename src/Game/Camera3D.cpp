// Camera3D.cpp
#include "Camera3D.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace {
    constexpr float DEFAULT_SPEED = 2.5f;
    constexpr float DEFAULT_SENSITIVITY = 0.1f;
    constexpr float DEFAULT_ZOOM = 45.0f;
}

Camera3D::Camera3D(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : position(position), worldUp(up), yaw(yaw), pitch(pitch),
    front(glm::vec3(0.0f, 0.0f, -1.0f)),
    movementSpeed(DEFAULT_SPEED),
    mouseSensitivity(DEFAULT_SENSITIVITY),
    zoom(DEFAULT_ZOOM) {
    updateCameraVectors();
}

Camera3D::~Camera3D() {
}

glm::mat4 Camera3D::GetViewMatrix() const {
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera3D::GetProjectionMatrix(float aspectRatio, float nearPlane, float farPlane) const {
    glm::mat4 proj = glm::perspective(glm::radians(zoom), aspectRatio, nearPlane, farPlane);
    proj[1][1] *= -1.0f;   // Vulkan's clip space Y is flipped vs OpenGL — see note below
    return proj;
}

void Camera3D::ProcessKeyboard(CameraMovement direction, float deltaTime) {
    float velocity = movementSpeed * deltaTime;
    switch (direction) {
    case CameraMovement::Forward:  position += front * velocity; break;
    case CameraMovement::Backward: position -= front * velocity; break;
    case CameraMovement::Left:     position -= right * velocity; break;
    case CameraMovement::Right:    position += right * velocity; break;
    case CameraMovement::Up:       position += worldUp * velocity; break;
    case CameraMovement::Down:     position -= worldUp * velocity; break;
    }
}

void Camera3D::ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch) {
    xOffset *= mouseSensitivity;
    yOffset *= mouseSensitivity;

    yaw += xOffset;
    pitch += yOffset;

    if (constrainPitch) {
        pitch = std::clamp(pitch, -89.0f, 89.0f);
    }

    updateCameraVectors();
}

void Camera3D::ProcessMouseScroll(float yOffset) {
    zoom -= yOffset;
    zoom = std::clamp(zoom, 1.0f, 45.0f);
}

void Camera3D::updateCameraVectors() {
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);

    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}