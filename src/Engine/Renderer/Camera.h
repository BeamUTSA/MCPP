#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    explicit Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f), float yaw = -90.0f,
                    float pitch = 0.0f);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float width, float height) const;

    void processMouseMovement(float xoffset, float yoffset);
    void translate(const glm::vec3& delta);

    [[nodiscard]] const glm::vec3& getPosition() const { return m_position; }
    [[nodiscard]] const glm::vec3& getFront() const { return m_front; }
    [[nodiscard]] const glm::vec3& getRight() const { return m_right; }
    [[nodiscard]] const glm::vec3& getUp() const { return m_up; }

    [[nodiscard]] float getFov() const { return m_fov; }
    void setFov(float fovDegrees) { m_fov = fovDegrees; }

    [[nodiscard]] float getMouseSensitivity() const { return m_mouseSensitivity; }
    void setMouseSensitivity(float sensitivity) { m_mouseSensitivity = sensitivity; }

private:
    void updateVectors();

    glm::vec3 m_position;
    glm::vec3 m_front{0.0f, 0.0f, -1.0f};
    glm::vec3 m_up{0.0f, 1.0f, 0.0f};
    glm::vec3 m_right{1.0f, 0.0f, 0.0f};

    float m_yaw{-90.0f};
    float m_pitch{0.0f};
    float m_fov{70.0f};
    float m_moveSpeed{6.0f};
    float m_mouseSensitivity{0.1f};
};
