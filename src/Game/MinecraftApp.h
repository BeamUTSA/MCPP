#pragma once
#define GLM_ENABLE_EXPERIMENTAL

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>

#include "Engine/Renderer/Camera.h"
#include "Engine/Renderer/Shader.h"
#include "Engine/ImGui/ImGuiLayer.hpp"
#include "Game/World/ChunkManager.h"
#include "Game/Rendering/TextureAtlas.h"
#include "Game/Player/PlayerController.h"

class MinecraftApp {
public:
    MinecraftApp() = default;
    ~MinecraftApp() = default;

    // Creates window, GL context, ImGui, shader, atlas, block DB, chunks, player
    bool init();

    // Main loop
    void run();

    // GLFW callbacks
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);

    // Accessors used by other systems
    [[nodiscard]] Camera& getCamera() { return m_camera; }
    [[nodiscard]] const Camera& getCamera() const { return m_camera; }

    [[nodiscard]] Shader* getShader() const { return m_shader.get(); }

    [[nodiscard]] GLFWwindow* getWindow() const { return m_window; }

    // Wireframe flag exposed for ImGui
    bool isWireframeEnabled() const { return m_wireframe; }
    void setWireframeEnabled(bool enabled) { m_wireframe = enabled; }

    // Pause mode flag exposed for ImGui and input
    bool isPaused() const { return m_paused; }
    void setPaused(bool paused);

    // Query a block by world position, used by Chunk/Player for logic
    uint8_t getBlock(glm::ivec3 worldPos) const;

private:
    // Per-frame helpers
    void processInput() const;
    void renderFrame();
    void renderTerrainTweakingPanel() const;

    // Window / timing
    GLFWwindow* m_window{nullptr};
    int   m_width{1280};
    int   m_height{720};
    float m_deltaTime{0.0f};
    float m_lastFrame{0.0f};

    // Core world state
    Camera                            m_camera{glm::vec3(6.5f, 5.0f, 10.0f), -135.0f, -20.0f};
    std::unique_ptr<ChunkManager>     m_chunkManager;
    std::unique_ptr<PlayerController> m_playerController;

    // Mouse state for look
    double m_lastMouseX{0.0};
    double m_lastMouseY{0.0};
    bool   m_firstMouse{true};

    // Rendering resources
    std::unique_ptr<Shader> m_shader;
    MCPP::TextureAtlas      m_textureAtlas;

    // ImGui
    ImGuiLayer m_imguiLayer;

    // Guard to avoid GL calls before glad is ready
    bool m_glFunctionsReady{false};

    // Wireframe toggle (default: solid fill)
    bool m_wireframe{false};

    // Pause state (default: not paused)
    bool m_paused{false};
    bool m_lastPauseKeyState{false}; // For detecting key press edges
    bool m_lastReloadKeyState{false}; // For detecting R key press edges
};
