#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "Engine/Renderer/Camera.h"
#include "Engine/Physics/Collision.h"

class ChunkManager; // Forward declaration

/**
 * PlayerController - Handles player movement, input, and physics.
 *
 * Manages:
 *   - WASD movement with acceleration/damping
 *   - Sprint detection (double-tap W)
 *   - Jumping & gravity
 *   - Simple collision against the voxel world via ChunkManager::getBlock
 */
class PlayerController {
public:
    PlayerController(Camera& camera, ChunkManager& chunkManager);

    void setWindow(GLFWwindow* window) { m_window = window; }

    // Called every frame from MinecraftApp
    void update(float deltaTime);

    // Mouse look
    void onMouseMoved(float xoffset, float yoffset);

    // Debug UI
    void renderImGui();

    [[nodiscard]] Camera& getCamera() { return m_camera; }
    [[nodiscard]] const Camera& getCamera() const { return m_camera; }

    // Set player FEET position (0, highestSolid+1, 0 style)
    void setFeetPosition(const glm::vec3& feetPos);

    // Handy so we can build camera spawn if needed
    float getEyeHeight() const { return m_playerEyeHeight; }

private:
    // Helpers
    void handleSprintDetection(float currentTime, int wKeyState);
    void applyGravity(float deltaTime);
    void resolveCollisions();    // Uses ChunkManager to stand on blocks
    void resolveHorizontalCollisions();

    void updateHitbox();

    // References
    Camera&       m_camera;
    ChunkManager& m_chunkManager;
    GLFWwindow*   m_window{nullptr};

    // Movement state
    glm::vec3 m_velocity{0.0f};  // World-space velocity
    bool      m_isGrounded{false};

    // Movement tuning
    float m_baseSpeed{5.0f};          // m/s
    float m_sprintMultiplier{1.8f};   // multiplier when sprinting
    float m_acceleration{20.0f};      // m/s^2
    float m_damping{8.0f};            // friction-like damping

    // Vertical motion
    float m_gravity{-20.0f};          // m/s^2 (negative)
    float m_jumpSpeed{8.0f};          // m/s
    float m_maxFallSpeed{50.0f};      // terminal velocity

    // Sprint detection (double-tap W)
    float m_doubleTapWindow{0.3f};    // seconds between taps
    float m_lastWPressTime{-1.0f};
    bool  m_wHeld{false};
    bool  m_sprinting{false};

    // Player collision hitbox
    AABB m_hitbox;

    // Player dimensions (Minecraft style: 0.6 x 1.8)
    float m_playerHalfWidth{0.3f};    // Half of 0.6
    float m_playerHalfHeight{0.9f};   // Half of 1.8
    float m_playerEyeHeight{1.62f};   // Eye height from feet
};
