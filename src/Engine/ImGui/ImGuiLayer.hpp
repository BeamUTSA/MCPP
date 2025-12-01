#pragma once

#include <GLFW/glfw3.h>

class ImGuiLayer {
public:
    ImGuiLayer() = default;
    ~ImGuiLayer();

    void init(GLFWwindow* window);
    void beginFrame();
    void renderDockspace();
    void renderPerformanceOverlay(float deltaTime);
    void endFrame();
    void shutdown();

private:
    bool m_initialized{false};
};
