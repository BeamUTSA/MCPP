#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

class Camera {
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Yaw;
    float Pitch;
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = -90.0f, float pitch = 0.0f);

    glm::mat4 getViewMatrix();
    glm::mat4 getProjectionMatrix(float width, float height);

    void processKeyboard(GLFWwindow* window, float deltaTime);
    void processMouseMovement(double xpos, double ypos, GLboolean constrainPitch = true);

private:
    bool firstMouse = true;
    double lastX = 0.0;
    double lastY = 0.0;
    void updateCameraVectors();
};

#endif
