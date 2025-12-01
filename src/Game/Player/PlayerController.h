#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "Engine/Renderer/Camera.h"

class PlayerController {
public:
    explicit PlayerController(Camera& camera);

    void setWindow(GLFWwindow* window) { m_window = window; }
    void update(float deltaTime);
    void onMouseMoved(float xoffset, float yoffset);
    void renderImGui();

private:
    void handleSprintDetection(float currentTime, int wKeyState);

    Camera& m_camera;
    GLFWwindow* m_window{nullptr};

    glm::vec3 m_velocity{0.0f};
    float m_acceleration{20.0f};
    float m_damping{8.0f};
    float m_baseSpeed{6.0f};
    float m_sprintMultiplier{1.8f};

    float m_doubleTapWindow{0.3f};
    float m_lastWPressTime{-1.0f};
    bool m_wHeld{false};
    bool m_sprinting{false};
};
