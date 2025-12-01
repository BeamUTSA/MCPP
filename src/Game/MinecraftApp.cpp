#include "Game/MinecraftApp.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <iostream>

namespace {
constexpr float SKY_BLUE_R = 0.529f;
constexpr float SKY_BLUE_G = 0.807f;
constexpr float SKY_BLUE_B = 0.921f;
}

bool MinecraftApp::init() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(m_width, m_height, "MCPP", nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetCursorPosCallback(m_window, mouseCallback);
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glViewport(0, 0, m_width, m_height);

    m_playerController.setWindow(m_window);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    if (!GLAD_GL_VERSION_4_5) {
        std::cerr << "OpenGL 4.5 is required but not supported by this system." << std::endl;
        return false;
    }

    m_glFunctionsReady = true;

    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);

    int framebufferWidth = m_width;
    int framebufferHeight = m_height;
    glfwGetFramebufferSize(m_window, &framebufferWidth, &framebufferHeight);
    framebufferWidth = std::max(1, framebufferWidth);
    framebufferHeight = std::max(1, framebufferHeight);
    m_width = framebufferWidth;
    m_height = framebufferHeight;
    if (m_glFunctionsReady && glad_glViewport) {
        glViewport(0, 0, framebufferWidth, framebufferHeight);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    m_imguiLayer.init(m_window);

    m_shader = Shader{"assets/shaders/core/vertex.glsl", "assets/shaders/core/fragment.glsl"};

    for (int x = 0; x < 3; ++x) {
        for (int z = 0; z < 3; ++z) {
            auto chunk = std::make_unique<Chunk>(glm::vec3(x * 3.0f, 0.0f, z * 3.0f));
            chunk->generate();
            m_chunks.push_back(std::move(chunk));
        }
    }

    return true;
}

void MinecraftApp::run() {
    while (!glfwWindowShouldClose(m_window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        m_deltaTime = currentFrame - m_lastFrame;
        m_lastFrame = currentFrame;

        processInput();

        m_imguiLayer.beginFrame();
        renderFrame();
        m_imguiLayer.renderDockspace();
        m_playerController.renderImGui();
        m_imguiLayer.renderPerformanceOverlay(m_deltaTime);
        m_imguiLayer.endFrame();

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }

    m_imguiLayer.shutdown();
    m_chunks.clear();
    glfwTerminate();
}

void MinecraftApp::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* app = static_cast<MinecraftApp*>(glfwGetWindowUserPointer(window));
    if (!app) {
        return;
    }

    if (app->m_glFunctionsReady && glad_glViewport) {
        glViewport(0, 0, width, height);
    }

    app->m_width = std::max(1, width);
    app->m_height = std::max(1, height);
}

void MinecraftApp::mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* app = static_cast<MinecraftApp*>(glfwGetWindowUserPointer(window));
    if (!app) return;

    if (app->m_firstMouse) {
        app->m_lastMouseX = xpos;
        app->m_lastMouseY = ypos;
        app->m_firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos - app->m_lastMouseX);
    float yoffset = static_cast<float>(app->m_lastMouseY - ypos);

    app->m_lastMouseX = xpos;
    app->m_lastMouseY = ypos;

    app->m_playerController.onMouseMoved(xoffset, yoffset);
}

void MinecraftApp::processInput() {
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(m_window, true);
    }

    m_playerController.update(m_deltaTime);
}

void MinecraftApp::renderFrame() {
    glClearColor(SKY_BLUE_R, SKY_BLUE_G, SKY_BLUE_B, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_shader.use();

    glm::mat4 projection = m_camera.getProjectionMatrix(static_cast<float>(m_width), static_cast<float>(m_height));
    glm::mat4 view = m_camera.getViewMatrix();
    m_shader.setMat4("projection", projection);
    m_shader.setMat4("view", view);
    m_shader.setVec3("lightDir", glm::normalize(glm::vec3(0.5f, 1.0f, 0.2f)));

    for (auto& chunk : m_chunks) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), chunk->getPosition());
        m_shader.setMat4("model", model);
        chunk->render();
    }
}
