// Camera3D.h
#pragma once
#include <glm/glm.hpp>

enum class CameraMovement {
    Forward,
    Backward,
    Left,
    Right,
    Up,
    Down,
    RotateLeft,
    RotateRight,
    Shift
};

class Camera3D {
public:
    Camera3D(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
        float yaw = -90.0f,
        float pitch = -45.0f);
    ~Camera3D();

    // Matrix generation for graphics pipeline integration
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspectRatio, float nearPlane = 0.1f, float farPlane = 100.0f) const;

    void ProcessOrbit(float deltaYaw, float deltaPitch);
    void ProcessOrbitZoom(float deltaDistance);
    glm::mat4 GetOrbitViewMatrix() const;
    void SnapRotateIso(bool clockwise);            // call once per Q/E press, not per frame
    void PanIso(glm::vec3 direction, float deltaTime); // WASD ground-plane panning
    void UpdateIso(float deltaTime);                // smooth the snap animation
    glm::mat4 GetIsoViewMatrix() const;

    // Input handling — called every frame from your input polling code
    void ProcessKeyboard(CameraMovement direction, float deltaTime, bool sprint = false);
    void ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);
    void ProcessMouseScroll(float yOffset);

    // Accessors
    glm::vec3 GetPosition() const { return position; }
    float GetZoom() const { return zoom; }
    enum class Mode { FreeFly, Isometric };
    float GetIsoYaw() const { return isoYaw; }
    void SetMode(Mode mode) { m_Mode = mode; }
    Mode GetMode() const { return m_Mode; }
    void SetIsoTarget(glm::vec3 pos) { isoTarget = pos; }
    glm::mat4 GetActiveViewMatrix() const;   // picks the right one internally


    //Setters
    void SetMovementSpeed(float speed) { movementSpeed = speed; }
    void SetMouseSensitivity(float s) { mouseSensitivity = s; }
    void SetIsoRotateSpeed(float s) { isoRotateSpeed = s; }
    void SetIsoDistance(float d) { isoDistance = d; }
    void SetIsoPitch(float p) { isoPitch = p; }
private:
    Mode m_Mode = Mode::Isometric;   // default to gameplay camera
    glm::vec3 target = glm::vec3(0.0f);   // the point being orbited
    float orbitDistance = 10.0f;
    float orbitYaw = -90.0f;
    float orbitPitch = 20.0f;
    glm::vec3 isoTarget = glm::vec3(0.0f);      // point on the ground the camera looks at
    float isoDistance = 20.0f;
    float isoPitch = 45.0f;                      // fixed downward angle
    float isoYaw = -45.0f;                        // current snapped facing
    float isoTargetYaw = -45.0f;                   // where we're snapping toward
    float isoRotateSpeed = 8.0f;                   // degrees/sec while animating the snap
	float sprintMultiplier = 2.0f;   // multiplier for free-fly speed when Shift is held    

    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw;
    float pitch;

    float movementSpeed;
    float mouseSensitivity;
    float rotationSpeed;
    float zoom;

    void updateCameraVectors();
};