#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // Include for glm::translate
#include <unordered_map>
#include <memory>
#include <queue>

#include "Game/World/Generation/Chunk.h"
#include "Game/World/Generation/SurfaceManager.h"

#include "Game/Player/PlayerController.h" // For player position

// Custom hash for glm::ivec3 to use as a key in std::unordered_map
template <>
struct std::hash<glm::ivec3> {
    size_t operator()(const glm::ivec3& v) const {
        const size_t h1 = hash<int>()(v.x);
        const size_t h2 = hash<int>()(v.y);
        const size_t h3 = hash<int>()(v.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

class MinecraftApp; // Forward declaration
namespace MCPP { class TextureAtlas; } // Forward declaration

class ChunkManager {
public:
    explicit ChunkManager(MinecraftApp& app, MCPP::TextureAtlas& atlas);
    ~ChunkManager();

        void update(const glm::vec3& playerPosition);
        void render() const;

        // Get block type at world coordinates
        uint8_t getBlock(int x, int y, int z) const;

        // Get block type at world coordinates, returns 0 if chunk not loaded
        uint8_t getBlock(const glm::ivec3& worldPos) const {
        return getBlock(worldPos.x, worldPos.y, worldPos.z);
        }

        // Returns highest solid block y at a world (x,z), or -1 if none.
        int getHighestSolidYAt(int worldX, int worldZ);

        // Access to surface manager for terrain tweaking
        SurfaceManager& getSurfaceManager() { return m_surfaceManager; }
        const SurfaceManager& getSurfaceManager() const { return m_surfaceManager; }

        // Reload all chunks (useful after changing terrain parameters)
        void reloadAllChunks();

private:
    MinecraftApp& m_app;
    MCPP::TextureAtlas& m_atlas;
    SurfaceManager m_surfaceManager;

    std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>> m_chunks;
    int m_renderDistance{12}; // Chunks in each direction from player

    std::queue<glm::ivec3> m_pendingChunks; // queue chunks so PC don't die ;/

    // Helper to get chunk coordinates from world coordinates
    glm::ivec3 getChunkCoords(const glm::vec3& worldPos) const;
    glm::ivec3 getBlockLocalCoords(const glm::vec3& worldPos) const;

    int getHighestSolidYAt(int worldX, int worldZ) const;

    void loadChunk(const glm::ivec3& chunkCoords);
    void unloadFarChunks(const glm::vec3& playerPosition);
    void processPendingChunks(int maxPerFrame);
};
