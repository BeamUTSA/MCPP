#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>

#include "Engine/Renderer/Camera.h"
#include "Engine/Renderer/Shader.h"
#include "Engine/ImGui/ImGuiLayer.hpp"
#include "Game/Player/PlayerController.h"
#include "Game/World/Chunk.h"

class MinecraftApp {
public:
    bool init();
    void run();

private:
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);

    void processInput();
    void renderFrame();

    GLFWwindow* m_window{nullptr};
    int m_width{1280};
    int m_height{720};

    float m_deltaTime{0.0f};
    float m_lastFrame{0.0f};

    Camera m_camera{glm::vec3(8.0f, 5.0f, 8.0f)};
    PlayerController m_playerController{m_camera};
    double m_lastMouseX{0.0};
    double m_lastMouseY{0.0};
    bool m_firstMouse{true};

    Shader m_shader;
    std::vector<std::unique_ptr<Chunk>> m_chunks;

    ImGuiLayer m_imguiLayer;
};
