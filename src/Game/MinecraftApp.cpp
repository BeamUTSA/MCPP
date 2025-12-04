#include "Game/MinecraftApp.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <iostream>
#include <imgui.h>

#include "World/Block/BlockDatabase.h"
#include "World/Generation/Noise.h"
#include "World/Generation/Surface.h"

namespace {
constexpr float SKY_BLUE_R = 0.529f;
constexpr float SKY_BLUE_G = 0.807f;
constexpr float SKY_BLUE_B = 0.921f;
}

bool MinecraftApp::init() {
    // --- GLFW init & window ---
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
    glfwSwapInterval(1); // V-sync

    // Set user pointer for static callbacks
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
    glfwSetCursorPosCallback(m_window, mouseCallback);

    // --- GLAD ---
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    m_glFunctionsReady = true;
    glViewport(0, 0, m_width, m_height);

    // --- GL state ---
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE); 

    // Capture mouse for FPS-style camera
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // --- ImGui ---
    m_imguiLayer.init(m_window);

    // --- Shader ---
    m_shader = std::make_unique<Shader>(
        "assets/shaders/core/vertex.glsl",
        "assets/shaders/core/fragment.glsl"
    );

    // --- Texture atlas ---
    if (!m_textureAtlas.load(
            "assets/textures/blocks/block_atlas.png",
            "assets/textures/blocks/atlas_mapping.json"
        )) {
        std::cerr << "Failed to load texture atlas" << std::endl;
        return false;
    }

    // --- Block database ---
    auto& blockDB = MCPP::BlockDatabase::instance();
    if (!blockDB.load("assets/textures/blocks/block_registry.json", m_textureAtlas)) {
        std::cerr << "Failed to load block database" << std::endl;
        return false;
    }

    // --- Chunk manager ---
    m_chunkManager = std::make_unique<ChunkManager>(*this, m_textureAtlas);

    // ---------------------------------------------------------------------
    // Determine spawn at (0, highestSolidY + 1, 0) in WORLD space
    // ---------------------------------------------------------------------
    const int spawnX = 0;
    const int spawnZ = 0;

    // First, make sure chunks around the spawn column are generated.
    // Y here is arbitrary; only X/Z matter for chunk selection.
    glm::vec3 tempSpawnProbe(spawnX + 0.5f, 64.0f, spawnZ + 0.5f);
    m_chunkManager->update(tempSpawnProbe);

    // Scan downward to find highest solid block in this column.
    int highestY = 0;
    bool foundSolid = false;
    for (int y = Chunk::CHUNK_HEIGHT - 1; y >= 0; --y) {
        uint8_t id = m_chunkManager->getBlock(spawnX, y, spawnZ);
        if (blockDB.isSolid(id)) {
            highestY = y;
            foundSolid = true;
            break;
        }
    }
    if (!foundSolid) {
        highestY = 0; // fallback if somehow empty
    }

    // Player feet go one block above the highest solid block.
    // Camera is at feet + eyeHeight (1.62f, matching PlayerController).
    constexpr float eyeHeight = 1.62f;
    glm::vec3 cameraSpawnPos(
        spawnX + 0.5f,
        static_cast<float>(highestY + 1) + eyeHeight,
        spawnZ + 0.5f
    );

    // --- Camera initial position & orientation ---
    // (reuse your nice diagonal yaw/pitch)
    m_camera = Camera{cameraSpawnPos, -135.0f, -30.0f};

    // --- Player controller (hooked into camera & chunks) ---
    m_playerController = std::make_unique<PlayerController>(m_camera, *m_chunkManager);
    m_playerController->setWindow(m_window);

    // Preload chunks around the *actual* camera/player position (radial)
    m_chunkManager->update(m_camera.getPosition());

    return true;
}

void MinecraftApp::run() {
    while (!glfwWindowShouldClose(m_window)) {
        const auto currentFrame = static_cast<float>(glfwGetTime());
        m_deltaTime = currentFrame - m_lastFrame;
        m_lastFrame = currentFrame;

        processInput();

        // Stream/generate/unload chunks based on player/camera position
        m_chunkManager->update(m_camera.Position);

        // --- ImGui frame ---
        m_imguiLayer.beginFrame();

        // Game rendering
        renderFrame();

        // ImGui UI
        m_imguiLayer.renderDockspace();
        if (m_playerController) {
            m_playerController->renderImGui();
        }
        m_imguiLayer.renderPerformanceOverlay(m_deltaTime);
        m_imguiLayer.renderDebugWindow(*this); // wireframe toggle lives here
        renderTerrainTweakingPanel(); // terrain generation tweaking panel

        m_imguiLayer.endFrame();

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }

    m_imguiLayer.shutdown();
    glfwTerminate();
}

void MinecraftApp::processInput() const {
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(m_window, true);
    }

    // TAB key toggles pause mode (non-const cast needed for toggle)
    auto* nonConstThis = const_cast<MinecraftApp*>(this);
    bool currentPauseKeyState = (glfwGetKey(m_window, GLFW_KEY_TAB) == GLFW_PRESS);
    if (currentPauseKeyState && !m_lastPauseKeyState) {
        // Key just pressed (edge detection)
        nonConstThis->setPaused(!m_paused);
    }
    nonConstThis->m_lastPauseKeyState = currentPauseKeyState;

    // R key reloads all chunks (useful for testing terrain changes)
    bool currentReloadKeyState = (glfwGetKey(m_window, GLFW_KEY_R) == GLFW_PRESS);
    if (currentReloadKeyState && !m_lastReloadKeyState) {
        // Key just pressed (edge detection)
        if (m_chunkManager) {
            nonConstThis->m_chunkManager->reloadAllChunks();
        }
    }
    nonConstThis->m_lastReloadKeyState = currentReloadKeyState;

    // Only update player controller when not paused
    if (!m_paused && m_playerController) {
        m_playerController->update(m_deltaTime);
    }
}

void MinecraftApp::renderFrame() {
    glClearColor(SKY_BLUE_R, SKY_BLUE_G, SKY_BLUE_B, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!m_shader) return;

    // Wireframe toggle: switch polygon mode each frame based on flag
    if (m_wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    m_shader->use();
    m_shader->setInt("blockAtlas", 0);
    m_textureAtlas.bind(0);

    // Debug: Verify shader uniform and texture binding (print once)
    static bool debugPrinted = false;
    if (!debugPrinted) {
        GLint currentProgram;
        glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
        GLint boundTexture;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTexture);
        printf("Render: Using program %d, bound texture %d\n", currentProgram, boundTexture);
        debugPrinted = true;
    }

    glm::mat4 projection = m_camera.getProjectionMatrix(
        static_cast<float>(m_width),
        static_cast<float>(m_height)
    );
    glm::mat4 view = m_camera.getViewMatrix();

    m_shader->setMat4("projection", projection);
    m_shader->setMat4("view", view);
    m_shader->setVec3("lightDir", glm::normalize(glm::vec3(0.5f, 1.0f, 0.2f)));

    // Update camera frustum for chunk culling
    m_camera.updateFrustum(static_cast<float>(m_width), static_cast<float>(m_height));

    m_chunkManager->render();
}

void MinecraftApp::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* app = static_cast<MinecraftApp*>(glfwGetWindowUserPointer(window));
    if (!app) return;

    app->m_width  = std::max(1, width);
    app->m_height = std::max(1, height);

    if (app->m_glFunctionsReady) {
        glViewport(0, 0, app->m_width, app->m_height);
    }
}

void MinecraftApp::mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* app = static_cast<MinecraftApp*>(glfwGetWindowUserPointer(window));
    if (!app) return;

    // Don't process mouse movement when paused
    if (app->m_paused) {
        // Update last position to prevent camera jump when unpausing
        app->m_lastMouseX = xpos;
        app->m_lastMouseY = ypos;
        app->m_firstMouse = false;
        return;
    }

    if (app->m_firstMouse) {
        app->m_lastMouseX = xpos;
        app->m_lastMouseY = ypos;
        app->m_firstMouse = false;
    }

    const float xoffset = static_cast<float>(xpos - app->m_lastMouseX);
    const float yoffset = static_cast<float>(app->m_lastMouseY - ypos); // reversed Y

    app->m_lastMouseX = xpos;
    app->m_lastMouseY = ypos;

    if (app->m_playerController) {
        app->m_playerController->onMouseMoved(xoffset, yoffset);
    }
}

uint8_t MinecraftApp::getBlock(glm::ivec3 worldPos) const {
    if (!m_chunkManager) return 0;
    return m_chunkManager->getBlock(worldPos.x, worldPos.y, worldPos.z);
}

void MinecraftApp::setPaused(bool paused) {
    m_paused = paused;

    // Toggle cursor mode based on pause state
    if (m_paused) {
        // Enable cursor for ImGui interaction
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    } else {
        // Disable cursor for FPS camera control
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        // Reset first mouse flag to prevent camera jump
        m_firstMouse = true;
    }
}

void MinecraftApp::renderTerrainTweakingPanel() const {
    if (!m_chunkManager) return;

    auto* surface = m_chunkManager->getSurfaceManager().getDefaultSurface();
    if (!surface) return;

    auto* terrainNoise = surface->getTerrainNoise();
    if (!terrainNoise) return;

    TerrainParams& params = terrainNoise->getParams();
    bool paramsChanged = false;

    ImGui::Begin("Terrain Generation", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("Press TAB to toggle pause/use this panel • R = reload all chunks");
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Continentalness (Tectonic Scale)", ImGuiTreeNodeFlags_DefaultOpen)) {
        paramsChanged |= ImGui::SliderInt("Octaves##cont", &params.continentalnessOctaves, 3, 10);
        paramsChanged |= ImGui::SliderFloat("Frequency##cont", &params.continentalnessFrequency, 0.00001f, 0.001f, "%.7f");
        paramsChanged |= ImGui::SliderFloat("Gain##cont", &params.continentalnessGain, 0.0f, 1.0f);
        paramsChanged |= ImGui::SliderFloat("Lacunarity##cont", &params.continentalnessLacunarity, 1.0f, 4.0f);
    }

    if (ImGui::CollapsingHeader("Erosion (Mountain Placement)", ImGuiTreeNodeFlags_DefaultOpen)) {
        paramsChanged |= ImGui::SliderInt("Octaves##ero", &params.erosionOctaves, 3, 10);
        paramsChanged |= ImGui::SliderFloat("Frequency##ero", &params.erosionFrequency, 0.0001f, 0.01f, "%.6f");
    }

    if (ImGui::CollapsingHeader("Ridges & Valleys (Ridged Peaks)", ImGuiTreeNodeFlags_DefaultOpen)) {
        paramsChanged |= ImGui::SliderInt("Octaves##pv", &params.peaksValleysOctaves, 4, 12);
        paramsChanged |= ImGui::SliderFloat("Frequency##pv", &params.peaksValleysFrequency, 0.0005f, 0.02f, "%.6f");
        paramsChanged |= ImGui::SliderFloat("Gain##pv", &params.peaksValleysGain, 0.4f, 1.8f);
    }

    if (ImGui::CollapsingHeader("Domain Warp (Continent Shape)", ImGuiTreeNodeFlags_DefaultOpen)) {
        paramsChanged |= ImGui::SliderInt("Octaves##dw", &params.domainWarpOctaves, 3, 12);
        paramsChanged |= ImGui::SliderFloat("Frequency##dw", &params.domainWarpFrequency, 0.0005f, 0.01f, "%.6f");
        paramsChanged |= ImGui::SliderFloat("Amplitude##dw", &params.domainWarpAmplitude, 0.0f, 600.0f, "%.1f");
    }

    if (ImGui::CollapsingHeader("Detail Noise", ImGuiTreeNodeFlags_DefaultOpen)) {
        paramsChanged |= ImGui::SliderInt("Octaves##det", &params.detailOctaves, 1, 8);
        paramsChanged |= ImGui::SliderFloat("Frequency##det", &params.detailFrequency, 0.005f, 0.06f, "%.4f");
    }

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Height Multipliers", ImGuiTreeNodeFlags_DefaultOpen)) {
        paramsChanged |= ImGui::SliderFloat("Ocean Depth", &params.oceanDepthMultiplier, 20.0f, 250.0f);
        paramsChanged |= ImGui::SliderFloat("Beach Height", &params.beachHeightMultiplier, 10.0f, 100.0f);
        paramsChanged |= ImGui::SliderFloat("Land Height", &params.landHeightMultiplier, 80.0f, 300.0f);
        paramsChanged |= ImGui::SliderFloat("Mountain Height", &params.mountainHeightMultiplier, 200.0f, 600.0f);
        paramsChanged |= ImGui::SliderFloat("Hill Height", &params.hillHeightMultiplier, 20.0f, 120.0f);
        paramsChanged |= ImGui::SliderFloat("Detail Height", &params.detailHeightMultiplier, 4.0f, 40.0f);
    }

    paramsChanged |= ImGui::SliderInt("Water Level", &params.waterLevel, 40, 100);

    ImGui::Separator();

    if (paramsChanged) {
        terrainNoise->updateNoiseGenerators();
        m_chunkManager->reloadAllChunks();  // now instant thanks to deferred system!
    }

    if (ImGui::Button("Reset to God-Tier Settings", ImVec2(200, 30))) {
        params = TerrainParams{};  // resets to the defaults we set in Noise.h (the perfect ones)
        terrainNoise->updateNoiseGenerators();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(Instant reset)");

    ImGui::Separator();

    if (ImGui::Button("Reload All Chunks", ImVec2(200, 30))) {
        m_chunkManager->reloadAllChunks();
    }  // ← THIS BRACE WAS MISSING BEFORE ←
    ImGui::SameLine();
    ImGui::TextDisabled("(R key)");

    ImGui::End();  // now always called every frame
}
