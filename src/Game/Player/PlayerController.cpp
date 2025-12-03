#define GLM_ENABLE_EXPERIMENTAL



#include <algorithm>
#include <glad/glad.h>
#include <glm/gtx/norm.hpp>
#include <imgui.h>
#include "Game/Player/PlayerController.h"
#include "Game/World/ChunkManager.h"
#include "Game/World/Block/BlockDatabase.h"
// ============================================================================
// CONSTRUCTOR
// ============================================================================

PlayerController::PlayerController(Camera& camera, ChunkManager& chunkManager)
    : m_camera(camera)
    , m_chunkManager(chunkManager) {
    /*
     * Initialize hitbox based on camera position.
     *
     * Camera is at eye level. Hitbox is centered on the body.
     *
     * [Camera] eye (1.62m from ground)
     *    |
     * [Center] body center (0.9m from ground)
     *    |
     * [Feet] ground (0m)
     */
    updateHitbox();
}

// ============================================================================
// UPDATE
// ============================================================================

void PlayerController::update(float deltaTime) {
    // Make sure we have a window for input
    if (!m_window) {
        m_window = glfwGetCurrentContext();
        if (!m_window) {
            return;
        }
    }

    const float currentTime = static_cast<float>(glfwGetTime());

    // --- Sprint Detection (double-tap W) ---
    const int wKeyState = glfwGetKey(m_window, GLFW_KEY_W);
    handleSprintDetection(currentTime, wKeyState);

    // --- Jump Input (SPACE) ---
    if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS && m_isGrounded) {
        m_velocity.y = m_jumpSpeed;
        m_isGrounded = false;
    }

    // --- Movement Input (WASD) ---
    glm::vec3 forward = m_camera.Front;
    forward.y = 0.0f;
    if (glm::length2(forward) > 0.0f) {
        forward = glm::normalize(forward);
    }

    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 inputDir(0.0f);

    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) inputDir += forward;
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) inputDir -= forward;
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) inputDir -= right;
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) inputDir += right;

    if (glm::length2(inputDir) > 0.0f) {
        inputDir = glm::normalize(inputDir);
    }

    // Desired horizontal velocity
    float targetSpeed = m_baseSpeed * (m_sprinting ? m_sprintMultiplier : 1.0f);
    glm::vec3 targetHorizontalVelocity = inputDir * targetSpeed;

    // Current horizontal velocity (ignore vertical)
    glm::vec3 horizontalVelocity{m_velocity.x, 0.0f, m_velocity.z};

    // Acceleration toward target
    glm::vec3 velocityDelta = targetHorizontalVelocity - horizontalVelocity;
    const float maxChange = m_acceleration * deltaTime;
    const float deltaLenSq = glm::length2(velocityDelta);

    if (deltaLenSq > maxChange * maxChange && deltaLenSq > 0.0f) {
        float deltaLen = std::sqrt(deltaLenSq);
        velocityDelta = (velocityDelta / deltaLen) * maxChange;
    }

    horizontalVelocity += velocityDelta;

    // Damping when no movement input
    if (glm::length2(targetHorizontalVelocity) < 0.0001f) {
        const float dampingFactor = std::clamp(1.0f - m_damping * deltaTime, 0.0f, 1.0f);
        horizontalVelocity *= dampingFactor;
    }

    m_velocity.x = horizontalVelocity.x;
    m_velocity.z = horizontalVelocity.z;

    // --- Vertical motion (gravity) ---
    applyGravity(deltaTime);

    // --- Integrate position ---
    glm::vec3 oldPos = m_camera.Position;
    glm::vec3 newPos = oldPos + m_velocity * deltaTime;
    m_camera.Position = newPos;

    // --- Update hitbox & resolve collisions with world ---
    updateHitbox();
    resolveCollisions();
}

// ============================================================================
// SPRINT DETECTION
// ============================================================================

void PlayerController::handleSprintDetection(float currentTime, int wKeyState) {
    /*
     * Double-tap W to sprint.
     *
     * Track when W is pressed. If pressed twice within m_doubleTapWindow,
     * enable sprinting. Sprinting stops when W is released.
     */
    if (wKeyState == GLFW_PRESS) {
        if (!m_wHeld) {
            // W just pressed this frame
            if (m_lastWPressTime > 0.0f &&
                (currentTime - m_lastWPressTime) < m_doubleTapWindow) {
                // Double-tap detected
                m_sprinting = true;
            }
            m_lastWPressTime = currentTime;
            m_wHeld = true;
        }
    } else {
        // W released
        m_wHeld = false;
        m_sprinting = false;
    }
}

// ============================================================================
// GRAVITY
// ============================================================================

void PlayerController::applyGravity(float deltaTime) {
    if (!m_isGrounded) {
        // Apply gravity when airborne
        m_velocity.y += m_gravity * deltaTime;
        if (m_velocity.y < -m_maxFallSpeed) {
            m_velocity.y = -m_maxFallSpeed;
        }
    } else {
        // When grounded, don't accumulate downward velocity
        if (m_velocity.y < 0.0f) {
            m_velocity.y = 0.0f;
        }
    }
}

// ============================================================================
// COLLISION RESOLUTION (vertical against blocks)
// ============================================================================

void PlayerController::resolveCollisions() {
    m_isGrounded = false;

    const glm::vec3 camPos = m_camera.Position;
    float feetY = camPos.y - m_playerEyeHeight;

    // ---------------------------------------------------------------------
    // 1) Absolute safety: never fall below a hard floor
    // ---------------------------------------------------------------------
    if (feetY < 0.0f) {
        float correction = 0.0f - feetY;
        m_camera.Position.y += correction;
        m_velocity.y = 0.0f;
        m_isGrounded = true;
        updateHitbox();
        return;
    }

    // ---------------------------------------------------------------------
    // 2) VERTICAL collision (Y-axis) - Check blocks under the player
    // ---------------------------------------------------------------------
    int bx = static_cast<int>(std::floor(camPos.x));
    int bz = static_cast<int>(std::floor(camPos.z));

    auto& blockDB = MCPP::BlockDatabase::instance();

    int startY = static_cast<int>(std::floor(feetY));
    int minY   = std::max(startY - 4, 0);

    for (int by = startY; by >= minY; --by) {
        uint8_t blockId = m_chunkManager.getBlock(bx, by, bz);

        if (blockId == 0 || !blockDB.isSolid(blockId)) {
            continue;
        }

        float blockTopY = static_cast<float>(by) + 1.0f;

        if (feetY < blockTopY) {
            float correction = blockTopY - feetY;
            m_camera.Position.y += correction;
            feetY += correction;

            if (m_velocity.y < 0.0f) {
                m_velocity.y = 0.0f;
            }

            m_isGrounded = true;
        }

        break; // Found ground, move to horizontal checks
    }

    // ---------------------------------------------------------------------
    // 3) HORIZONTAL collision (X and Z axes) - Prevent walking into blocks
    // ---------------------------------------------------------------------
    resolveHorizontalCollisions();

    updateHitbox();
}

void PlayerController::resolveHorizontalCollisions() {
    const glm::vec3 camPos = m_camera.Position;
    float feetY = camPos.y - m_playerEyeHeight;
    float headY = camPos.y;

    auto& blockDB = MCPP::BlockDatabase::instance();

    // Player's horizontal radius (adjust to your player size)
    const float playerRadius = 0.3f;

    // Check blocks around the player at multiple heights
    // This checks at feet, mid-body, and head level
    std::vector<float> checkHeights = {feetY, feetY + 0.5f, headY - 0.1f};

    for (float checkY : checkHeights) {
        int blockY = static_cast<int>(std::floor(checkY));

        // Check in all 4 cardinal directions (and optionally diagonals)
        std::vector<glm::ivec3> offsetsToCheck = {
            {1, 0, 0},   // +X (East)
            {-1, 0, 0},  // -X (West)
            {0, 0, 1},   // +Z (South)
            {0, 0, -1},  // -Z (North)
            // Optionally add diagonal checks:
            {1, 0, 1},   // +X+Z (Southeast)
            {1, 0, -1},  // +X-Z (Northeast)
            {-1, 0, 1},  // -X+Z (Southwest)
            {-1, 0, -1}  // -X-Z (Northwest)
        };

        for (const auto& offset : offsetsToCheck) {
            // Block position to check
            int bx = static_cast<int>(std::floor(camPos.x)) + offset.x;
            int by = blockY + offset.y;
            int bz = static_cast<int>(std::floor(camPos.z)) + offset.z;

            uint8_t blockId = m_chunkManager.getBlock(bx, by, bz);

            // Skip air and non-solid blocks
            if (blockId == 0 || !blockDB.isSolid(blockId)) {
                continue;
            }

            // Block bounds (each block is 1x1x1 centered at integer coordinates)
            float blockMinX = static_cast<float>(bx);
            float blockMaxX = static_cast<float>(bx + 1);
            float blockMinZ = static_cast<float>(bz);
            float blockMaxZ = static_cast<float>(bz + 1);

            // Player bounds (circular collision using radius)
            float playerMinX = camPos.x - playerRadius;
            float playerMaxX = camPos.x + playerRadius;
            float playerMinZ = camPos.z - playerRadius;
            float playerMaxZ = camPos.z + playerRadius;

            // Check for overlap (AABB collision)
            bool overlapX = playerMaxX > blockMinX && playerMinX < blockMaxX;
            bool overlapZ = playerMaxZ > blockMinZ && playerMinZ < blockMaxZ;

            if (overlapX && overlapZ) {
                // We're colliding! Calculate penetration depth in each axis
                float penetrationLeft   = playerMaxX - blockMinX; // How far we penetrated from left
                float penetrationRight  = blockMaxX - playerMinX; // How far we penetrated from right
                float penetrationFront  = playerMaxZ - blockMinZ; // How far we penetrated from front
                float penetrationBack   = blockMaxZ - playerMinZ; // How far we penetrated from back

                // Find the smallest penetration (that's our push-out direction)
                float minPenetration = std::min({penetrationLeft, penetrationRight,
                                                  penetrationFront, penetrationBack});

                // Push player out in the direction of least penetration
                if (minPenetration == penetrationLeft) {
                    // Push player to the left (negative X)
                    m_camera.Position.x = blockMinX - playerRadius - 0.001f;
                    m_velocity.x = std::min(0.0f, m_velocity.x); // Stop positive X velocity
                }
                else if (minPenetration == penetrationRight) {
                    // Push player to the right (positive X)
                    m_camera.Position.x = blockMaxX + playerRadius + 0.001f;
                    m_velocity.x = std::max(0.0f, m_velocity.x); // Stop negative X velocity
                }
                else if (minPenetration == penetrationFront) {
                    // Push player to the front (negative Z)
                    m_camera.Position.z = blockMinZ - playerRadius - 0.001f;
                    m_velocity.z = std::min(0.0f, m_velocity.z); // Stop positive Z velocity
                }
                else { // penetrationBack
                    // Push player to the back (positive Z)
                    m_camera.Position.z = blockMaxZ + playerRadius + 0.001f;
                    m_velocity.z = std::max(0.0f, m_velocity.z); // Stop negative Z velocity
                }
            }
        }
    }
}

// ============================================================================
// HITBOX
// ============================================================================

void PlayerController::updateHitbox() {
    /*
     * Update hitbox position based on current camera position.
     *
     * Camera is at eye level. Hitbox center is below that.
     *
     * Hitbox center Y = Camera Y - EyeHeight + HalfHeight
     *                 = Camera Y - 1.62 + 0.9
     *                 = Camera Y - 0.72
     */
    const glm::vec3& camPos = m_camera.Position;

    glm::vec3 hitboxCenter{
        camPos.x,
        camPos.y - m_playerEyeHeight + m_playerHalfHeight,
        camPos.z
    };

    m_hitbox.updatePosition(hitboxCenter,
                            m_playerHalfWidth,
                            m_playerHalfHeight,
                            m_playerHalfWidth);
}

void PlayerController::setFeetPosition(const glm::vec3& feetPos) {
    // Camera is at eye height above feet
    glm::vec3 camPos = feetPos;
    camPos.y += m_playerEyeHeight;

    m_camera.setPosition(camPos);

    // Also update hitbox to match new camera position
    updateHitbox();
}

// ============================================================================
// MOUSE LOOK
// ============================================================================

void PlayerController::onMouseMoved(float xoffset, float yoffset) {
    m_camera.processMouseMovement(xoffset, yoffset);
}

// ============================================================================
// IMGUI DEBUG PANEL
// ============================================================================

void PlayerController::renderImGui() {
    if (ImGui::Begin("Player Controller")) {
        // --- Movement Settings ---
        ImGui::TextUnformatted("Movement");
        ImGui::Separator();
        ImGui::SliderFloat("Base Speed", &m_baseSpeed, 1.0f, 20.0f, "%.1f m/s");
        ImGui::SliderFloat("Sprint Multiplier", &m_sprintMultiplier, 1.0f, 3.0f, "%.1fx");
        ImGui::SliderFloat("Acceleration", &m_acceleration, 5.0f, 40.0f, "%.1f m/s²");
        ImGui::SliderFloat("Damping", &m_damping, 1.0f, 20.0f, "%.1f");

        ImGui::Spacing();
        ImGui::TextUnformatted("Vertical Motion");
        ImGui::Separator();
        ImGui::SliderFloat("Gravity", &m_gravity, -30.0f, -1.0f, "%.1f");
        ImGui::SliderFloat("Jump Speed", &m_jumpSpeed, 2.0f, 20.0f, "%.1f");
        ImGui::SliderFloat("Max Fall Speed", &m_maxFallSpeed, 10.0f, 80.0f, "%.1f");

        ImGui::Spacing();
        ImGui::TextUnformatted("Sprint");
        ImGui::Separator();
        ImGui::SliderFloat("Double-tap Window", &m_doubleTapWindow, 0.1f, 0.6f, "%.2f s");
        ImGui::Text("Sprinting: %s", m_sprinting ? "Yes" : "No");

        ImGui::Spacing();
        ImGui::TextUnformatted("Debug Info");
        ImGui::Separator();

        const glm::vec3& pos = m_camera.Position;
        ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
        ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", m_velocity.x, m_velocity.y, m_velocity.z);
        ImGui::Text("Speed (horizontal): %.2f m/s",
                    glm::length(glm::vec3(m_velocity.x, 0.0f, m_velocity.z)));
        ImGui::Text("Grounded: %s", m_isGrounded ? "Yes" : "No");

        ImGui::Spacing();
        ImGui::Text("Hitbox Min: (%.2f, %.2f, %.2f)",
                    m_hitbox.min.x, m_hitbox.min.y, m_hitbox.min.z);
        ImGui::Text("Hitbox Max: (%.2f, %.2f, %.2f)",
                    m_hitbox.max.x, m_hitbox.max.y, m_hitbox.max.z);
    }
    ImGui::End();
}
