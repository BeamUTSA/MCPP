#pragma once

struct GLFWwindow;
class MinecraftApp;

class ImGuiLayer {
public:
    ImGuiLayer() = default;
    ~ImGuiLayer();

    void init(GLFWwindow* window);

    void beginFrame();
    void renderDockspace();
    void renderPerformanceOverlay(float deltaTime);

    // New: debug window for engine toggles (wireframe, etc.)
    void renderDebugWindow(MinecraftApp& app);

    void endFrame();
    void shutdown();

private:
    bool m_initialized{false};
};
