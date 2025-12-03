#include "Game/MinecraftApp.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <iostream>

#include "World/Block/BlockDatabase.h"

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
    glEnable(GL_CULL_FACE_MODE); // keeping your working value

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
        if (m_chunkManager) {
            m_chunkManager->update(m_camera.getPosition());
        }

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

    if (m_playerController) {
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

    // Bind texture atlas
    m_textureAtlas.bind(0);
    m_shader->setInt("blockAtlas", 0);

    // Render chunks via ChunkManager
    if (m_chunkManager) {
        m_chunkManager->render();
    }
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
