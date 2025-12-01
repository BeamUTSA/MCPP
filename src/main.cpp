#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec2.hpp>
#include <iostream>
#include <vector>

// Window settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Camera
Camera camera(glm::vec3(8.0f, 5.0f, 8.0f));

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    camera.processMouseMovement(xpos, ypos);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    camera.processKeyboard(window, deltaTime);

    // Toggle fullscreen with F11
    static bool wasF11Pressed = false;
    bool isF11Pressed = glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS;

    if (isF11Pressed && !wasF11Pressed) {
        static int windowedWidth = SCR_WIDTH;
        static int windowedHeight = SCR_HEIGHT;
        static int windowedPosX = 100;
        static int windowedPosY = 100;

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        if (glfwGetWindowMonitor(window) == nullptr) {
            glfwGetWindowSize(window, &windowedWidth, &windowedHeight);
            glfwGetWindowPos(window, &windowedPosX, &windowedPosY);
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        } else {
            glfwSetWindowMonitor(window, nullptr, windowedPosX, windowedPosY, windowedWidth, windowedHeight, 0);
        }
    }
    wasF11Pressed = isF11Pressed;
}

struct Block {
    glm::vec2 faces[6];
};

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "MCPP", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // OpenGL configuration
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Create shader
    Shader shader("core/vertex.glsl", "core/fragment.glsl");

    // Create and bind SSBO
    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    // For now, let's just create a dummy block data array
    Block blocks[2]; // 0 = air, 1 = grass
    blocks[1].faces[0] = glm::vec2(0, 0); // Top
    blocks[1].faces[1] = glm::vec2(0, 0); // Bottom
    blocks[1].faces[2] = glm::vec2(0, 0); // Front
    blocks[1].faces[3] = glm::vec2(0, 0); // Back
    blocks[1].faces[4] = glm::vec2(0, 0); // Right
    blocks[1].faces[5] = glm::vec2(0, 0); // Left
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(blocks), blocks, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);


    // Create chunks (3x3 grid)
    std::vector<Chunk*> chunks;
    for (int x = 0; x < 3; x++) {
        for (int z = 0; z < 3; z++) {
            Chunk* chunk = new Chunk(glm::vec3(x * 16, 0, z * 16));
            chunk->generate();
            chunks.push_back(chunk);
        }
    }

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // Render
        glClearColor(0.5f, 0.7f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        // Set matrices and light direction
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glm::mat4 projection = camera.getProjectionMatrix(width, height);
        glm::mat4 view = camera.getViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        shader.setVec3("lightDir", glm::vec3(0.5f, 1.0f, 0.2f));


        // Render all chunks
        for (Chunk* chunk : chunks) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), chunk->position);
            shader.setMat4("model", model);
            chunk->render();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteBuffers(1, &ssbo);
    for (Chunk* chunk : chunks) {
        delete chunk;
    }

    glfwTerminate();
    return 0;
}