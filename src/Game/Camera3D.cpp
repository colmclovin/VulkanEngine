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
    rotationSpeed(DEFAULT_SPEED),
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

void Camera3D::ProcessKeyboard(CameraMovement direction, float deltaTime, bool sprint) {
    float speed = sprint ? movementSpeed * sprintMultiplier : movementSpeed;
    float velocity = speed * deltaTime;
	float rotateVelocity = rotationSpeed * deltaTime;
    glm::vec3 flatFront = glm::normalize(glm::vec3(front.x, 0.0f, front.z));


    switch (direction) {
    case CameraMovement::Forward:  position += flatFront * velocity; break;
    case CameraMovement::Backward: position -= flatFront * velocity; break;
    case CameraMovement::Left:     position -= right * velocity; break;
    case CameraMovement::Right:    position += right * velocity; break;
    case CameraMovement::Up:       position += worldUp * velocity; break;
    case CameraMovement::Down:     position -= worldUp * velocity; break;
    case CameraMovement::RotateLeft: ProcessOrbit(rotateVelocity, 0); break;
    case CameraMovement::RotateRight: ProcessOrbit(-rotateVelocity, 0); break;
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
void Camera3D::ProcessOrbit(float deltaYaw, float deltaPitch) {
    orbitYaw += deltaYaw;
    orbitPitch += deltaPitch;
    orbitPitch = std::clamp(orbitPitch, -89.0f, 89.0f);   // avoid gimbal flip at the poles
}

void Camera3D::ProcessOrbitZoom(float deltaDistance) {
    orbitDistance -= deltaDistance;
    orbitDistance = std::clamp(orbitDistance, 1.0f, 100.0f);
}

glm::mat4 Camera3D::GetOrbitViewMatrix() const {
    glm::vec3 orbitPos;
    orbitPos.x = target.x + orbitDistance * cos(glm::radians(orbitPitch)) * cos(glm::radians(orbitYaw));
    orbitPos.y = target.y + orbitDistance * sin(glm::radians(orbitPitch));
    orbitPos.z = target.z + orbitDistance * cos(glm::radians(orbitPitch)) * sin(glm::radians(orbitYaw));

    return glm::lookAt(orbitPos, target, worldUp);
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


void Camera3D::SnapRotateIso(bool clockwise) {
    isoTargetYaw += clockwise ? 45.0f : -45.0f;
}

void Camera3D::UpdateIso(float deltaTime) {
    // smoothly rotate isoYaw toward isoTargetYaw for a nice AoE-style swing instead of an instant jump
    float diff = isoTargetYaw - isoYaw;
    float step = isoRotateSpeed * 45.0f * deltaTime;  // scale however feels right
    if (std::abs(diff) <= step) {
        isoYaw = isoTargetYaw;
    }
    else {
        isoYaw += (diff > 0.0f ? step : -step);
    }
}

void Camera3D::PanIso(glm::vec3 direction, float deltaTime) {
    // pan along screen-relative ground axes, so WASD stays intuitive as the view rotates
    glm::vec3 forward = glm::normalize(glm::vec3(cos(glm::radians(isoYaw)), 0.0f, sin(glm::radians(isoYaw))));
    glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));

    isoTarget += (forward * direction.z + right * direction.x) * movementSpeed * deltaTime;
}

glm::mat4 Camera3D::GetIsoViewMatrix() const {
    glm::vec3 isoPos;
    isoPos.x = isoTarget.x + isoDistance * cos(glm::radians(isoPitch)) * cos(glm::radians(isoYaw));
    isoPos.y = isoTarget.y + isoDistance * sin(glm::radians(isoPitch));
    isoPos.z = isoTarget.z + isoDistance * cos(glm::radians(isoPitch)) * sin(glm::radians(isoYaw));

    return glm::lookAt(isoPos, isoTarget, worldUp);
}

// Camera3D.cpp
glm::mat4 Camera3D::GetActiveViewMatrix() const {
    return (m_Mode == Mode::Isometric) ? GetIsoViewMatrix() : GetViewMatrix();
}