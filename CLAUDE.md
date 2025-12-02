# CLAUDE.md - AI Assistant Guide for MCPP

## Project Overview

**MCPP** (Minecraft C++ Remake) is a production-grade, clean-architecture implementation of a Minecraft-like voxel engine built with modern C++20, OpenGL 4.5, and a clear separation between Engine and Game layers.

**Current Status**: Early development - Phase 1-2 complete (window, rendering, camera, basic chunk rendering)

**Target**: A high-performance, scalable Minecraft clone with 144+ FPS, infinite terrain, greedy meshing, and optional ECS architecture.

---

## Architecture Philosophy

### Core Design Principles

1. **Engine/Game Separation**: Clear boundaries between reusable engine code and game-specific logic
2. **Layer-Based Architecture**: New features should be added as Layers or ECS Systems to keep `MinecraftApp.cpp` minimal
3. **Modern C++20**: Use C++20 features, RAII, smart pointers, `[[nodiscard]]`, and const-correctness
4. **Performance First**: Target 144+ FPS with 16-24 chunk render distance before moving to advanced features
5. **Incremental Development**: Follow the roadmap in `MCPP.md` - each phase has clear acceptance criteria

### Directory Structure

```
MCPP/
├── src/
│   ├── main.cpp                    # Entry point - minimal, just instantiates MinecraftApp
│   ├── Engine/                     # Reusable engine components
│   │   ├── Renderer/
│   │   │   ├── Camera.h/cpp        # Camera with view/projection matrices
│   │   │   └── Shader.h/cpp        # Shader compilation and uniform management
│   │   └── ImGui/
│   │       └── ImGuiLayer.cpp      # ImGui integration for debug overlays
│   └── Game/                       # Minecraft-specific game logic
│       ├── MinecraftApp.h/cpp      # Main application class (window, loop, rendering)
│       ├── Player/
│       │   └── PlayerController.h/cpp  # Input handling, movement, sprint detection
│       └── World/
│           └── Chunk.h/cpp         # Chunk generation, meshing, rendering
│
├── Game/                           # Future game-specific code (placeholder directories)
│   └── World/
│       └── Block/                  # Future: BlockDatabase, block definitions
│
├── assets/
│   ├── shaders/
│   │   ├── core/                   # Current shaders (vertex.glsl, fragment.glsl, uniforms.glsl)
│   │   └── chunk/                  # Future: chunk-specific shaders, compute shaders
│   ├── textures/
│   │   ├── blocks/                 # Future: block textures for atlas
│   │   ├── gui/                    # Future: UI textures
│   │   └── items/                  # Future: item textures
│   └── sounds/                     # Future: audio assets
│
├── vendor/                         # Third-party dependencies
│   ├── glad/                       # OpenGL function loader
│   ├── glfw/                       # Window/input library
│   ├── glm/                        # Math library (vectors, matrices)
│   ├── imgui/                      # Immediate-mode GUI (docking branch)
│   ├── entt/                       # ECS library (future use)
│   ├── spdlog/                     # Logging library (future use)
│   ├── stb/                        # Image loading (future use)
│   └── FastNoiseLite/              # Noise generation for terrain (future use)
│
├── cmake/                          # CMake helper files
├── CMakeLists.txt                  # Main build configuration
├── MCPP.md                         # Development roadmap with 18 phases
└── CLAUDE.md                       # This file - AI assistant guide
```

---

## Build System

### Technology Stack

- **Build Tool**: CMake 3.21+ with Ninja generator
- **C++ Standard**: C++20 (strict, no extensions)
- **Graphics API**: OpenGL 4.5 Core Profile
- **Platform**: Cross-platform (Linux primary, Windows/Mac supported)

### Key Dependencies

| Library | Version | Purpose | Integration Method |
|---------|---------|---------|-------------------|
| GLFW | 3.4 | Window/Input | Vendored or FetchContent |
| GLAD | Generated | OpenGL loader | Vendored (compiled as library) |
| GLM | 1.0.1+ | Math library | Header-only, vendored or FetchContent |
| ImGui | docking branch | Debug UI | Vendored sources compiled directly |
| EnTT | Latest | ECS (future) | Header-only, vendored |
| spdlog | Latest | Logging (future) | Vendored |
| FastNoiseLite | Latest | Terrain gen (future) | Header-only, vendored |
| stb_image | Latest | Texture loading (future) | Header-only, vendored |

### CMake Configuration

**Build Options:**
- `ENABLE_ASAN`: Enable AddressSanitizer (Debug builds only, default OFF)
- `ENABLE_LTO`: Enable Link-Time Optimization (Release builds, default ON)

**Build Targets:**
```bash
# Configure (Debug)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

# Configure (Release with LTO)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_LTO=ON

# Build
cmake --build build

# Run (assets are auto-copied to build directory)
./build/MCPP
```

**Important Build Notes:**
1. **Assets Auto-Copy**: CMake automatically copies `assets/` to build directory after each build
2. **Vendor Handling**: CMake checks for vendored libraries first, falls back to FetchContent if missing
3. **Platform-Specific Linking**:
   - Linux: Links GL, dl, pthread, X11, Xi, Xrandr, Xinerama, Xcursor
   - Windows: Links opengl32
   - Mac: (to be added)

### Adding New Source Files

**When adding a new `.cpp` file**, you MUST update `CMakeLists.txt`:

```cmake
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/Game/MinecraftApp.cpp
    src/Game/Player/PlayerController.cpp
    src/Engine/ImGui/ImGuiLayer.cpp
    src/Engine/Renderer/Camera.cpp
    src/Engine/Renderer/Shader.cpp
    src/Game/World/Chunk.cpp
    # Add your new file here!
    src/Game/World/ChunkManager.cpp  # Example
)
```

**Header files (.h)** do NOT need to be added to CMakeLists.txt - only `.cpp` files.

---

## Coding Conventions

### Naming Conventions

| Element | Convention | Example |
|---------|-----------|---------|
| Classes | PascalCase | `MinecraftApp`, `ChunkManager`, `PlayerController` |
| Member variables | m_camelCase with `m_` prefix | `m_window`, `m_deltaTime`, `m_chunks` |
| Functions/Methods | camelCase | `processInput()`, `renderFrame()`, `buildMesh()` |
| Constants | SCREAMING_SNAKE_CASE or constexpr | `CHUNK_SIZE`, `SKY_BLUE_R` |
| Local variables | camelCase | `currentFrame`, `xoffset`, `vertexCount` |
| Namespaces | lowercase | `namespace detail { }` |
| Files | PascalCase matching class name | `MinecraftApp.h`, `Chunk.cpp` |

### Header Guards

**Use `#pragma once`** (preferred for this codebase):
```cpp
#pragma once

#include <glad/glad.h>
// ...
```

**Exception**: `Chunk.h` currently uses traditional guards (`#ifndef CHUNK_H`) - this is acceptable but `#pragma once` is preferred for new files.

### File Structure

**Header File Pattern (`.h`)**:
```cpp
#pragma once

// System includes first
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

// Local includes second
#include "Engine/Renderer/Camera.h"
#include "Game/World/Chunk.h"

class MyClass {
public:
    // Public interface first
    bool init();
    void update(float deltaTime);

    // Getters with [[nodiscard]]
    [[nodiscard]] const glm::vec3& getPosition() const { return m_position; }

private:
    // Private members last
    void internalHelper();

    // Member variables with m_ prefix
    glm::vec3 m_position{0.0f};
    float m_deltaTime{0.0f};
    std::unique_ptr<Shader> m_shader;
};
```

**Implementation File Pattern (`.cpp`)**:
```cpp
#include "Game/MyClass.h"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

// Anonymous namespace for file-local constants/helpers
namespace {
constexpr float DEFAULT_SPEED = 5.0f;
}

bool MyClass::init() {
    // Implementation
}
```

### Modern C++ Patterns

1. **Smart Pointers**:
   - Use `std::unique_ptr` for exclusive ownership (see `m_shader`, `m_chunks` in `MinecraftApp`)
   - Use `std::make_unique` for construction
   - Use raw pointers only for non-owning references (e.g., `GLFWwindow*`)

2. **Const-Correctness**:
   - Mark read-only methods as `const`
   - Use `const&` for pass-by-reference parameters
   - Use `[[nodiscard]]` for getters that return important values

3. **Initialization**:
   - Use in-class member initializers where possible: `float m_deltaTime{0.0f};`
   - Use initializer lists in constructors for complex initialization

4. **Error Handling**:
   - Return `bool` for success/failure operations (e.g., `MinecraftApp::init()`)
   - Use `std::cerr` for error messages
   - Check and handle errors from external libraries (GLFW, GLAD, OpenGL)

### OpenGL Conventions

1. **Initialization Order** (CRITICAL):
   ```cpp
   glfwInit()
   glfwCreateWindow()
   glfwMakeContextCurrent()
   gladLoadGLLoader()  // MUST be before any OpenGL calls!
   glViewport()        // Now safe to call
   ```

2. **Member Variable Naming for OpenGL Objects**:
   - Use descriptive names: `m_program`, `VAO`, `VBO`, `EBO`
   - Use `unsigned int` for OpenGL object IDs

3. **Shader Files**:
   - Version: `#version 450 core`
   - Use `#include "uniforms.glsl"` for shared uniforms
   - Vertex attributes use explicit locations: `layout (location = 0) in vec3 aPos;`

### ImGui Debug Overlays

**Always use ImGui for debugging**:
- Performance overlay (FPS counter) - see `ImGuiLayer::renderPerformanceOverlay()`
- Dockspace for flexible UI layout - see `ImGuiLayer::renderDockspace()`
- Component-specific debug windows (e.g., `PlayerController::renderImGui()`)

**Pattern for adding ImGui to a component**:
```cpp
// In header
void renderImGui();

// In implementation
void MyComponent::renderImGui() {
    ImGui::Begin("My Component");
    ImGui::Text("Some Value: %.2f", m_someValue);
    ImGui::SliderFloat("Speed", &m_speed, 1.0f, 10.0f);
    ImGui::End();
}
```

---

## Current Implementation Details

### MinecraftApp (Core Application)

**File**: `src/Game/MinecraftApp.h/cpp`

**Key Responsibilities**:
- Window creation and management (GLFW)
- Main game loop
- Input event routing
- Rendering pipeline coordination
- ImGui frame management

**Critical Flow**:
```
main()
  → MinecraftApp::init()
      → glfwInit(), create window
      → gladLoadGLLoader()  // MUST be after glfwMakeContextCurrent!
      → Initialize Camera, PlayerController
      → Load shaders
      → Generate initial chunks
  → MinecraftApp::run()
      Loop:
        → processInput()
        → renderFrame()
            → Clear screen (Minecraft sky blue: 0.529, 0.807, 0.921)
            → Render all chunks
        → ImGui frame (dockspace, overlays)
        → Swap buffers, poll events
```

**Important Callbacks**:
- `framebufferSizeCallback()`: Updates viewport and window dimensions
- `mouseCallback()`: Routes mouse movement to PlayerController

### Camera System

**File**: `src/Engine/Renderer/Camera.h/cpp`

**Features**:
- View and projection matrix generation
- Mouse look (yaw/pitch)
- Translation (movement)
- Configurable FOV and mouse sensitivity

**Key Methods**:
- `getViewMatrix()`: Returns view matrix for shader
- `getProjectionMatrix(width, height)`: Returns perspective projection
- `processMouseMovement(xoffset, yoffset)`: Updates yaw/pitch
- `translate(delta)`: Moves camera in world space

**Default Settings**:
- Position: `(6.5, 5.0, 10.0)` or `(8.0, 5.0, 8.0)` depending on chunk config
- Yaw: -135° (looking toward negative X/Z)
- Pitch: -20° (looking down slightly)
- FOV: 70°
- Mouse sensitivity: 0.1

### Player Controller

**File**: `src/Game/Player/PlayerController.h/cpp`

**Features**:
- WASD movement with smooth acceleration/damping
- Double-tap W to sprint (sprint multiplier: 1.8x)
- ImGui debug panel showing position, velocity, speed

**Movement System**:
- Physics-based with acceleration (20.0) and damping (8.0)
- Base speed: 6.0 units/sec
- Sprint speed: 10.8 units/sec (6.0 × 1.8)
- Double-tap window: 0.3 seconds

**Integration**:
- Owns a reference to Camera (not a copy)
- Receives `GLFWwindow*` pointer via `setWindow()`
- Called every frame with `update(deltaTime)`

### Chunk System

**File**: `src/Game/World/Chunk.h/cpp`

**Current Implementation**:
- Chunk size: 16×16×16 blocks
- Simple procedural generation (currently just a ground plane)
- Naive meshing: one quad per visible face, face culling against adjacent blocks
- Indexed rendering with VBO, EBO, VAO

**Mesh Vertex Format** (per vertex):
```
Position (3 floats) | Color (3 floats) | Normal (3 floats)
Layout location 0   | Layout location 1 | Layout location 2
```

**Block Storage**:
- 3D array: `blocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE]`
- Block type: `unsigned char` (0 = air, 1 = grass, etc.)

**Face Culling**:
- `isBlockSolid(x, y, z)`: Checks if block is not air
- `addFace(pos, face)`: Adds quad vertices for one face (6 faces: -X, +X, -Y, +Y, -Z, +Z)

**Rendering**:
```cpp
chunk->render();
  → glBindVertexArray(VAO)
  → glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, 0)
```

### Shader System

**File**: `src/Engine/Renderer/Shader.h/cpp`

**Current Shaders**:
- **Vertex**: `assets/shaders/core/vertex.glsl`
- **Fragment**: `assets/shaders/core/fragment.glsl`
- **Uniforms**: `assets/shaders/core/uniforms.glsl`

**Shader Uniforms** (defined in `uniforms.glsl`):
```glsl
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 lightDir;
```

**Lighting**:
- Simple directional light: `normalize(vec3(0.5, 1.0, 0.2))`
- Lambertian diffuse shading
- Ambient: 20% of base color
- Diffuse: Based on normal dot light direction

**Usage Pattern**:
```cpp
m_shader->use();
m_shader->setMat4("projection", projection);
m_shader->setMat4("view", view);
m_shader->setVec3("lightDir", glm::normalize(glm::vec3(0.5, 1.0, 0.2)));
// Per-chunk:
m_shader->setMat4("model", model);
chunk->render();
```

---

## Development Workflow

### Git Workflow

1. **Branching**: Work on feature branches named `claude/feature-description-{session-id}`
2. **Commits**: Commit after every completed phase (see roadmap in `MCPP.md`)
3. **Commit Messages**: Use imperative mood, focus on "why" not "what"
   - Good: "Fix dockspace passthrough so scene stays visible"
   - Bad: "Changed ImGui settings"

### Testing Workflow

**Manual Testing Checklist** (run after significant changes):
1. Window opens with Minecraft sky blue background
2. ImGui dockspace is visible and functional
3. FPS counter shows in top-left
4. WASD movement works smoothly
5. Mouse look rotates camera
6. Double-tap W enables sprint (visible in PlayerController debug panel)
7. Chunks render correctly with lighting
8. ESC closes the application

### Adding New Features

**Follow this process**:

1. **Read the Roadmap**: Check `MCPP.md` to see what phase you're in
2. **Plan**: Identify which files need changes
3. **Layer-Based Design**:
   - Add new features as Layers (like `ImGuiLayer`) or ECS Systems
   - Keep `MinecraftApp.cpp` minimal - it should orchestrate, not implement
4. **Update CMakeLists.txt**: Add new `.cpp` files to the executable
5. **Test**: Manually verify the acceptance criteria from the roadmap
6. **Commit**: Commit with clear message referencing the phase/feature

### Common Tasks

#### Adding a New Class

1. **Create header** in appropriate directory (e.g., `src/Game/World/ChunkManager.h`)
2. **Use `#pragma once`** at the top
3. **Follow naming conventions**: PascalCase class name, m_prefix for members
4. **Create implementation** (e.g., `src/Game/World/ChunkManager.cpp`)
5. **Update CMakeLists.txt**:
   ```cmake
   add_executable(${PROJECT_NAME}
       # ...
       src/Game/World/ChunkManager.cpp  # Add this line
   )
   ```
6. **Include in relevant files** (e.g., `MinecraftApp.h`)

#### Adding a New Shader

1. **Create shader files** in `assets/shaders/` subdirectory
2. **Use `#version 450 core`** at the top
3. **Include shared uniforms**: `#include "uniforms.glsl"` if needed
4. **Load in code**:
   ```cpp
   m_myShader = std::make_unique<Shader>(
       "assets/shaders/myshader/vertex.glsl",
       "assets/shaders/myshader/fragment.glsl"
   );
   ```
5. **No build system changes needed** - assets are auto-copied

#### Adding ImGui Debug Panel

1. **Add method to your class**:
   ```cpp
   void MyClass::renderImGui() {
       ImGui::Begin("My Feature");
       ImGui::Text("Status: %s", m_status.c_str());
       ImGui::SliderFloat("Parameter", &m_param, 0.0f, 100.0f);
       ImGui::End();
   }
   ```
2. **Call from `MinecraftApp::run()`**:
   ```cpp
   m_imguiLayer.beginFrame();
   renderFrame();
   m_imguiLayer.renderDockspace();
   m_playerController.renderImGui();
   m_myFeature.renderImGui();  // Add this
   m_imguiLayer.renderPerformanceOverlay(m_deltaTime);
   m_imguiLayer.endFrame();
   ```

---

## Future Roadmap Integration

**See `MCPP.md` for the full 18-phase roadmap.** Key upcoming features:

### Next Immediate Priorities (Phases 3-6)

1. **Phase 3**: Block database & texture atlas (auto-generated 4096×4096)
2. **Phase 4**: Infinite terrain with Perlin/Simplex noise (use FastNoiseLite)
3. **Phase 5**: Improve chunk meshing (current implementation is naive)
4. **Phase 6**: ChunkManager for loading/unloading chunks based on render distance

### Critical Future Systems

- **ECS Integration** (Phase 12): Migrate to EnTT-based architecture
- **Threaded Meshing** (Phase 13): Move chunk generation to worker threads
- **Compute Shaders** (Phase 14): GPU-based meshing for 10,000+ chunks
- **Greedy Meshing** (Phase 7): Reduce vertex count by 3-5× with optimal quad merging

---

## Common Pitfalls & Solutions

### Build Issues

**Problem**: "undefined reference" or linker errors
- **Solution**: Make sure you added the `.cpp` file to `CMakeLists.txt`

**Problem**: Can't find header files
- **Solution**: Check `#include` paths - use `"Game/..."` or `"Engine/..."` format

**Problem**: Assets not found at runtime
- **Solution**: Run from build directory (`./build/MCPP`), assets are auto-copied there

### OpenGL Issues

**Problem**: Black screen or no rendering
- **Solution**: Check that `gladLoadGLLoader()` was called BEFORE any OpenGL function calls

**Problem**: Segfault on glViewport or other GL calls
- **Solution**: Ensure `glfwMakeContextCurrent()` is called before GLAD initialization

**Problem**: Shader compilation errors
- **Solution**: Check shader version is `#version 450 core` and file paths are correct

### Chunk Rendering Issues

**Problem**: Chunks render incorrectly or have holes
- **Solution**: Verify face culling logic in `Chunk::buildMesh()` - check adjacency tests

**Problem**: Performance degrades with many chunks
- **Solution**: You've hit the naive meshing limit - proceed to Phase 7 (greedy meshing)

### ImGui Issues

**Problem**: ImGui windows not appearing
- **Solution**: Check that `m_imguiLayer.beginFrame()` and `endFrame()` wrap all ImGui calls

**Problem**: Scene disappears behind dockspace
- **Solution**: Ensure dockspace passthrough flag is set correctly in `ImGuiLayer`

---

## AI Assistant Guidelines

### When Helping with This Codebase

1. **Always check the roadmap** (`MCPP.md`) before suggesting new features
2. **Follow the layer-based architecture** - don't bloat `MinecraftApp`
3. **Update CMakeLists.txt** when adding new `.cpp` files
4. **Use the established naming conventions** - m_prefix for members, PascalCase for classes
5. **Add ImGui debug panels** for any new gameplay systems
6. **Test thoroughly** - this is a graphics project, visual verification is critical
7. **Keep commits focused** - one feature per commit, following the roadmap phases

### What to Avoid

- Don't add features out of order (e.g., don't add multiplayer before chunk management exists)
- Don't create new rendering systems - extend the existing `Shader` class
- Don't bypass the Engine/Game separation
- Don't use raw pointers for ownership (use `std::unique_ptr` or `std::shared_ptr`)
- Don't skip error checking for external libraries (GLFW, OpenGL, shader compilation)

### Useful Commands for Analysis

```bash
# Find all source files
find src -name "*.cpp" -o -name "*.h"

# Search for a class definition
grep -r "class ChunkManager" src/

# Check what's in CMakeLists.txt
grep "add_executable" CMakeLists.txt

# See current build configuration
cat build/CMakeCache.txt | grep CMAKE_BUILD_TYPE
```

---

## Quick Reference

### File Locations

| Purpose | Path |
|---------|------|
| Main entry point | `src/main.cpp` |
| Application class | `src/Game/MinecraftApp.h/cpp` |
| Camera | `src/Engine/Renderer/Camera.h/cpp` |
| Player movement | `src/Game/Player/PlayerController.h/cpp` |
| Chunk system | `src/Game/World/Chunk.h/cpp` |
| Shader wrapper | `src/Engine/Renderer/Shader.h/cpp` |
| Current vertex shader | `assets/shaders/core/vertex.glsl` |
| Current fragment shader | `assets/shaders/core/fragment.glsl` |
| Build config | `CMakeLists.txt` |
| Development roadmap | `MCPP.md` |

### Key Constants

```cpp
// Chunk.h
CHUNK_SIZE = 16        // X and Z dimensions
CHUNK_HEIGHT = 16      // Y dimension

// MinecraftApp.cpp
SKY_BLUE_R = 0.529f
SKY_BLUE_G = 0.807f
SKY_BLUE_B = 0.921f

// Camera.h defaults
m_fov = 70.0f
m_mouseSensitivity = 0.1f

// PlayerController.h defaults
m_baseSpeed = 6.0f
m_sprintMultiplier = 1.8f
m_acceleration = 20.0f
m_damping = 8.0f
```

---

## Contact & Resources

- **GitHub**: Repository URL (check git remotes)
- **Roadmap**: See `MCPP.md` for full 18-phase development plan
- **OpenGL Docs**: https://docs.gl/
- **GLFW Docs**: https://www.glfw.org/documentation.html
- **ImGui Docs**: https://github.com/ocornut/imgui
- **GLM Docs**: https://glm.g-truc.net/

---

**Last Updated**: 2025-12-02
**Project Phase**: 1-2 (Window, Camera, Basic Chunks)
**Next Phase**: 3 (Block Database & Texture Atlas)
