#define GLM_ENABLE_EXPERIMENTAL

#include "Game/Player/PlayerController.h"

#include <algorithm>

#include <glm/gtx/norm.hpp>
#include <imgui.h>

PlayerController::PlayerController(Camera& camera) : m_camera(camera) {}

void PlayerController::update(float deltaTime) {
    if (!m_window) {
        m_window = glfwGetCurrentContext();
        if (!m_window) {
            return;
        }
    }

    const float currentTime = static_cast<float>(glfwGetTime());
    const int wKeyState = glfwGetKey(m_window, GLFW_KEY_W);
    handleSprintDetection(currentTime, wKeyState);

    glm::vec3 inputDir{0.0f};
    if (wKeyState == GLFW_PRESS) inputDir += m_camera.getFront();
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) inputDir -= m_camera.getFront();
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) inputDir -= m_camera.getRight();
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) inputDir += m_camera.getRight();
    if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) inputDir += m_camera.getUp();
    if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) inputDir -= m_camera.getUp();

    if (glm::length2(inputDir) > 0.0001f) {
        inputDir = glm::normalize(inputDir);
    }

    const float targetSpeed = m_baseSpeed * (m_sprinting ? m_sprintMultiplier : 1.0f);
    const glm::vec3 targetVelocity = inputDir * targetSpeed;

    glm::vec3 velocityDelta = targetVelocity - m_velocity;
    const float maxChange = m_acceleration * deltaTime;
    const float deltaLength = glm::length(velocityDelta);
    if (deltaLength > maxChange && deltaLength > 0.0f) {
        velocityDelta = (velocityDelta / deltaLength) * maxChange;
    }

    m_velocity += velocityDelta;

    if (glm::length2(targetVelocity) < 0.0001f) {
        const float dampingFactor = std::clamp(1.0f - m_damping * deltaTime, 0.0f, 1.0f);
        m_velocity *= dampingFactor;
    }

    m_camera.translate(m_velocity * deltaTime);
}

void PlayerController::onMouseMoved(float xoffset, float yoffset) {
    m_camera.processMouseMovement(xoffset, yoffset);
}

void PlayerController::renderImGui() {
    if (ImGui::Begin("Player Controller")) {
        ImGui::TextUnformatted("Movement");
        ImGui::Separator();
        ImGui::SliderFloat("Base Speed", &m_baseSpeed, 1.0f, 20.0f, "%.1f");
        ImGui::SliderFloat("Sprint Multiplier", &m_sprintMultiplier, 1.0f, 3.0f, "%.1fx");
        ImGui::SliderFloat("Acceleration", &m_acceleration, 5.0f, 40.0f, "%.1f");
        ImGui::SliderFloat("Damping", &m_damping, 1.0f, 20.0f, "%.1f");
        ImGui::Checkbox("Sprinting", &m_sprinting);

        ImGui::Spacing();
        ImGui::TextUnformatted("View");
        ImGui::Separator();
        float fov = m_camera.getFov();
        if (ImGui::SliderFloat("FOV", &fov, 60.0f, 110.0f, "%.0f deg")) {
            m_camera.setFov(fov);
        }

        float sensitivity = m_camera.getMouseSensitivity();
        if (ImGui::SliderFloat("Mouse Sensitivity", &sensitivity, 0.05f, 1.0f, "%.2f")) {
            m_camera.setMouseSensitivity(sensitivity);
        }

        ImGui::Spacing();
        ImGui::Text("Position: %.1f, %.1f, %.1f", m_camera.getPosition().x, m_camera.getPosition().y,
                    m_camera.getPosition().z);
    }
    ImGui::End();
}

void PlayerController::handleSprintDetection(float currentTime, int wKeyState) {
    if (wKeyState == GLFW_PRESS) {
        if (!m_wHeld) {
            if (m_lastWPressTime > 0.0f && (currentTime - m_lastWPressTime) < m_doubleTapWindow) {
                m_sprinting = true;
            }
            m_lastWPressTime = currentTime;
            m_wHeld = true;
        }
    } else {
        m_wHeld = false;
        m_sprinting = false;
    }
}
