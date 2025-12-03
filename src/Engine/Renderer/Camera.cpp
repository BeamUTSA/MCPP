/**
 * Camera.cpp - Camera Implementation
 */

#include "Engine/Renderer/Camera.h"
#include <algorithm>
#include <cmath>

// Default camera values
namespace CameraDefaults {
    constexpr float YAW         = -90.f;   // Looking along -Z
    constexpr float PITCH       = 0.f;
    constexpr float SPEED       = 10.f;    // Units per second
    constexpr float SENSITIVITY = 0.1f;    // Mouse sensitivity
    constexpr float ZOOM        = 70.f;    // Field of view in degrees
}

// ============================================================================
// CONSTRUCTORS
// ============================================================================

Camera::Camera(glm::vec3 position, float yaw, float pitch)
    : Position{position}
    , Front{0.f, 0.f, -1.f}
    , Up{0.f, 1.f, 0.f}
    , Right{1.f, 0.f, 0.f}
    , WorldUp{0.f, 1.f, 0.f}
    , Yaw{yaw}
    , Pitch{pitch}
    , MovementSpeed{CameraDefaults::SPEED}
    , MouseSensitivity{CameraDefaults::SENSITIVITY}
    , Zoom{CameraDefaults::ZOOM}
{
    updateCameraVectors();
}

Camera::Camera(float posX, float posY, float posZ, float yaw, float pitch)
    : Camera{glm::vec3{posX, posY, posZ}, yaw, pitch}
{
    // Delegating constructor
}

// ============================================================================
// MATRIX GENERATION
// ============================================================================

glm::mat4 Camera::getViewMatrix() const {
    /*
     * VIEW MATRIX
     * ===========
     * Transforms world coordinates into camera ("view") space.
     *
     * glm::lookAt creates a matrix that:
     *   1. Translates the world so the camera is at origin
     *   2. Rotates so the camera looks down -Z
     */
    return glm::lookAt(Position, Position + Front, Up);
}

glm::mat4 Camera::getProjectionMatrix(float width, float height,
                                       float nearPlane, float farPlane) const
{
    /*
     * PROJECTION MATRIX (Perspective)
     * ================================
     * Transforms view space into clip space.
     *
     * Parameters:
     *   - fov:    Vertical field of view (in RADIANS)
     *   - aspect: Width / height of viewport
     *   - near:   Near clipping plane
     *   - far:    Far clipping plane (render distance)
     */
    float aspect = width / height;
    return glm::perspective(glm::radians(Zoom), aspect, nearPlane, farPlane);
}

glm::mat4 Camera::getViewProjectionMatrix(float width, float height,
                                           float nearPlane, float farPlane) const
{
    return getProjectionMatrix(width, height, nearPlane, farPlane) * getViewMatrix();
}

// ============================================================================
// INPUT HANDLING
// ============================================================================

void Camera::processKeyboard(CameraMovement direction, float deltaTime) {
    /*
     * Movement is frame-rate independent via deltaTime.
     */
    const float velocity = MovementSpeed * deltaTime;

    switch (direction) {
        case CameraMovement::Forward:
            Position += Front * velocity;
            break;
        case CameraMovement::Backward:
            Position -= Front * velocity;
            break;
        case CameraMovement::Left:
            Position -= Right * velocity;
            break;
        case CameraMovement::Right:
            Position += Right * velocity;
            break;
        case CameraMovement::Up:
            Position += WorldUp * velocity;
            break;
        case CameraMovement::Down:
            Position -= WorldUp * velocity;
            break;
    }
}

void Camera::processMouseMovement(float xOffset, float yOffset, bool constrainPitch) {
    /*
     * MOUSE LOOK
     * ==========
     * xOffset affects Yaw (looking left/right)
     * yOffset affects Pitch (looking up/down)
     */
    xOffset *= MouseSensitivity;
    yOffset *= MouseSensitivity;

    Yaw   += xOffset;
    Pitch += yOffset;

    // Constrain pitch to prevent camera flipping
    if (constrainPitch) {
        Pitch = std::clamp(Pitch, -89.f, 89.f);
    }

    // Recalculate camera vectors with new angles
    updateCameraVectors();
}

void Camera::processMouseScroll(float yOffset) {
    /*
     * ZOOM (Field of View)
     * ====================
     * Smaller FOV = more zoomed in
     * Larger FOV = more zoomed out
     */
    Zoom -= yOffset;
    Zoom = std::clamp(Zoom, 1.f, 120.f);
}

void Camera::translate(const glm::vec3& offset) {
    Position += offset;
}

// ============================================================================
// FRUSTUM
// ============================================================================

void Camera::updateFrustum(float width, float height) {

    /*
     * Extract frustum planes from the combined view-projection matrix.
     * Call this once per frame before doing any culling.
     */
    glm::mat4 viewProjection = getViewProjectionMatrix(width, height);
    m_frustum.updateFromViewProjection(viewProjection);
}

// ============================================================================
// PRIVATE HELPERS
// ============================================================================

void Camera::updateCameraVectors() {
    /*
     * EULER ANGLES TO VECTORS
     * =======================
     * Convert Yaw (horizontal) and Pitch (vertical) angles into
     * direction vectors using spherical coordinates.
     */
    glm::vec3 front;
    front.x = std::cos(glm::radians(Yaw)) * std::cos(glm::radians(Pitch));
    front.y = std::sin(glm::radians(Pitch));
    front.z = std::sin(glm::radians(Yaw)) * std::cos(glm::radians(Pitch));

    Front = glm::normalize(front);

    /*
     * RIGHT AND UP VECTORS
     * ====================
     * Derived from Front using cross products.
     */
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up    = glm::normalize(glm::cross(Right, Front));
}