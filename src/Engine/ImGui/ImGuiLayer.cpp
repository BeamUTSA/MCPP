#include "ImGuiLayer.hpp"

#include "Game/MinecraftApp.h"      // for MinecraftApp in renderDebugWindow

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

ImGuiLayer::~ImGuiLayer() {
    shutdown();
}

void ImGuiLayer::init(GLFWwindow* window) {
    if (m_initialized) {
        return;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450");

    m_initialized = true;
}

void ImGuiLayer::beginFrame() {
    if (!m_initialized) {
        return;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::renderDockspace() {
    if (!m_initialized) {
        return;
    }

    // Use a passthrough dockspace so the 3D view remains visible behind ImGui.
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                   ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_MenuBar |
                                   ImGuiWindowFlags_NoBackground;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("Dockspace", nullptr, windowFlags);
    ImGui::PopStyleVar(3);

    const ImGuiID dockspaceId = ImGui::GetID("MainDockspace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();
}

void ImGuiLayer::renderPerformanceOverlay(float deltaTime) {
    if (!m_initialized) {
        return;
    }

    const float  fps      = deltaTime > 0.0f ? 1.0f / deltaTime : 0.0f;
    const float  frameMs  = deltaTime * 1000.0f;
    const double frameNs  = static_cast<double>(deltaTime) * 1'000'000'000.0;

    const float distanceFromEdge = 10.0f;
    ImGui::SetNextWindowPos(ImVec2(distanceFromEdge, distanceFromEdge), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.35f);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav;

    if (ImGui::Begin("PerformanceOverlay", nullptr, flags)) {
        ImGui::TextUnformatted("Frame Timings");
        ImGui::Separator();
        ImGui::Text("FPS: %.1f", fps);
        ImGui::Text("Frame: %.3f ms", frameMs);
        ImGui::Text("Frame: %.0f ns", frameNs);
    }

    ImGui::End();
}

void ImGuiLayer::renderDebugWindow(MinecraftApp& app) {
    if (!m_initialized) {
        return;
    }

    if (ImGui::Begin("Render Debug")) {
        bool wireframe = app.isWireframeEnabled();
        if (ImGui::Checkbox("Wireframe mode", &wireframe)) {
            app.setWireframeEnabled(wireframe);
        }
    }

    ImGui::End();
}

void ImGuiLayer::endFrame() {
    if (!m_initialized) {
        return;
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backupCurrentContext = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backupCurrentContext);
    }
}

void ImGuiLayer::shutdown() {
    if (!m_initialized) {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
}
