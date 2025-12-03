/**
 * Camera.h - FPS-Style Camera for Voxel Engine
 *
 * This camera provides:
 *   - Position and orientation (yaw/pitch)
 *   - View matrix for rendering
 *   - Direction vectors for frustum construction
 *   - Keyboard/mouse input handling
 *   - Integrated frustum for culling
 */

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Engine/Renderer/Frustum.h"

/**
 * Camera movement directions.
 * Used with processKeyboard() for clean input handling.
 */
enum class CameraMovement {
    Forward,
    Backward,
    Left,
    Right,
    Up,      // For flying/creative mode
    Down
};

/**
 * FPS-style camera with Euler angles (yaw/pitch).
 *
 * COORDINATE SYSTEM:
 *   +X = Right
 *   +Y = Up
 *   +Z = Backward (out of screen) - OpenGL convention
 *
 * So "Forward" is -Z direction.
 */
class Camera {
public:
    // --- Camera Attributes (public for direct access) ---
    glm::vec3 Position;
    glm::vec3 Front;    // Direction camera is looking
    glm::vec3 Up;       // Camera's up vector (not world up!)
    glm::vec3 Right;    // Camera's right vector
    glm::vec3 WorldUp;  // World up (usually {0,1,0})

    // --- Euler Angles (in degrees) ---
    float Yaw;          // Rotation around Y axis (look left/right)
    float Pitch;        // Rotation around X axis (look up/down)

    // --- Camera Options ---
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;         // Field of view (for zooming)

    // --- Constructors ---

    /**
     * Create camera with vectors.
     */
    Camera(glm::vec3 position = glm::vec3(0.f, 0.f, 0.f),
           float yaw = -90.f,    // Looking along -Z by default
           float pitch = 0.f);

    /**
     * Create camera with scalar values.
     */
    Camera(float posX, float posY, float posZ,
           float yaw, float pitch);

    // --- Core Functions ---

    /**
     * Get the view matrix for rendering.
     * Pass this to your shaders as the "view" uniform.
     */
    [[nodiscard]]
    glm::mat4 getViewMatrix() const;

    /**
     * Get a projection matrix (convenience function).
     *
     * @param width      Screen width in pixels
     * @param height     Screen height in pixels
     * @param nearPlane  Near clip distance
     * @param farPlane   Far clip distance (render distance)
     */
    [[nodiscard]]
    glm::mat4 getProjectionMatrix(float width, float height,
                                   float nearPlane = 0.1f,
                                   float farPlane = 500.f) const;

    /**
     * Get combined view-projection matrix.
     * Useful for frustum extraction via matrix method.
     */
    [[nodiscard]]
    glm::mat4 getViewProjectionMatrix(float width, float height,
                                       float nearPlane = 0.1f,
                                       float farPlane = 500.f) const;

    // --- Input Handling ---

    /**
     * Process keyboard input for movement.
     *
     * @param direction   Which way to move
     * @param deltaTime   Time since last frame (for smooth movement)
     */
    void processKeyboard(CameraMovement direction, float deltaTime);

    /**
     * Process mouse movement for looking around.
     *
     * @param xOffset        Mouse X movement (pixels)
     * @param yOffset        Mouse Y movement (pixels)
     * @param constrainPitch Prevent camera from flipping (default: true)
     */
    void processMouseMovement(float xOffset, float yOffset,
                              bool constrainPitch = true);

    /**
     * Process scroll wheel for zooming.
     *
     * @param yOffset  Scroll amount
     */
    void processMouseScroll(float yOffset);

    /**
     * Translate the camera by an offset.
     *
     * @param offset  Movement vector to add to position
     */
    void translate(const glm::vec3& offset);

    // --- Frustum ---

    /**
     * Update the frustum planes from current view/projection.
     * Call this once per frame before culling.
     *
     * @param width   Screen width
     * @param height  Screen height
     */
    void updateFrustum(float width, float height);

    /**
     * Get the frustum for culling tests.
     */
    [[nodiscard]]
    const Frustum& getFrustum() const { return m_frustum; }

    // --- Getters (for code that prefers methods over direct access) ---

    [[nodiscard]] const glm::vec3& getPosition() const { return Position; }
    [[nodiscard]] const glm::vec3& getFront() const { return Front; }
    [[nodiscard]] const glm::vec3& getUp() const { return Up; }
    [[nodiscard]] const glm::vec3& getRight() const { return Right; }

    [[nodiscard]] float getYaw() const { return Yaw; }
    [[nodiscard]] float getPitch() const { return Pitch; }

    [[nodiscard]] float getFov() const { return Zoom; }
    [[nodiscard]] float getZoom() const { return Zoom; }
    [[nodiscard]] float getFovRadians() const { return glm::radians(Zoom); }
    [[nodiscard]] float getMouseSensitivity() const { return MouseSensitivity; }

    // --- Setters ---

    void setPosition(const glm::vec3& pos) { Position = pos; }
    void setFov(float fov) { Zoom = fov; }
    void setZoom(float zoom) { Zoom = zoom; }
    void setMouseSensitivity(float sensitivity) { MouseSensitivity = sensitivity; }

private:
    Frustum m_frustum;

    /**
     * Recalculate Front, Right, Up vectors from Euler angles.
     * Called whenever yaw or pitch changes.
     */
    void updateCameraVectors();
};