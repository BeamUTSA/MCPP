#include "Game/World/ChunkManager.h"
#include "Game/MinecraftApp.h"
#include "Game/Rendering/TextureAtlas.h"
#include "Engine/Physics/Collision.h"
#include "Game/World/Block/BlockDatabase.h"

#include <imgui.h>

#include <iostream>


ChunkManager::ChunkManager(MinecraftApp& app, MCPP::TextureAtlas& atlas)
    : m_app(app)
    , m_atlas(atlas)
    , m_surfaceManager(1337u)   // world seed; later we can pass from app
{
    // ...
}


ChunkManager::~ChunkManager() {
    // Chunks are managed by unique_ptr, so they will be deallocated automatically
}

glm::ivec3 ChunkManager::getChunkCoords(const glm::vec3& worldPos) const {
    return glm::ivec3(
        static_cast<int>(std::floor(worldPos.x / Chunk::CHUNK_SIZE)),
        0, // Y-coordinate for chunk is always 0 for now, as chunks are vertical slices
        static_cast<int>(std::floor(worldPos.z / Chunk::CHUNK_SIZE))
    );
}

glm::ivec3 ChunkManager::getBlockLocalCoords(const glm::vec3& worldPos) const {
    // Ensure positive modulo for block coordinates
    int localX = static_cast<int>(worldPos.x) % Chunk::CHUNK_SIZE;
    if (localX < 0) localX += Chunk::CHUNK_SIZE;
    int localZ = static_cast<int>(worldPos.z) % Chunk::CHUNK_SIZE;
    if (localZ < 0) localZ += Chunk::CHUNK_SIZE;

    return glm::ivec3(
        localX,
        static_cast<int>(worldPos.y),
        localZ
    );
}

int ChunkManager::getHighestSolidYAt(int worldX, int worldZ) const {
    auto& db = MCPP::BlockDatabase::instance();

    for (int y = Chunk::CHUNK_HEIGHT - 1; y >= 0; --y) {
        uint8_t id = getBlock(worldX, y, worldZ); // already handles missing chunks as air
        if (db.isSolid(id)) {
            return y;
        }
    }
    return -1; // no solid block in this column
}

void ChunkManager::loadChunk(const glm::ivec3& chunkCoords) {
    if (m_chunks.find(chunkCoords) == m_chunks.end()) {
        // Create empty chunk and store it
        auto chunk = std::make_unique<Chunk>(chunkCoords);
        m_chunks[chunkCoords] = std::move(chunk);

        // Defer heavy work
        m_pendingChunks.push(chunkCoords);
    }
}

void ChunkManager::unloadFarChunks(const glm::vec3& playerPosition) {
    glm::ivec3 playerChunkCoords = getChunkCoords(playerPosition);

    for (auto it = m_chunks.begin(); it != m_chunks.end(); ) {
        const glm::ivec3& chunkCoords = it->first;
        if (std::abs(chunkCoords.x - playerChunkCoords.x) > m_renderDistance ||
            std::abs(chunkCoords.z - playerChunkCoords.z) > m_renderDistance) {
            it = m_chunks.erase(it);
        } else {
            ++it;
        }
    }
}

void ChunkManager::update(const glm::vec3& playerPosition) {
    glm::ivec3 playerChunkCoords = getChunkCoords(playerPosition);

    // Enqueue new chunks around player
    for (int x = -m_renderDistance; x <= m_renderDistance; ++x) {
        for (int z = -m_renderDistance; z <= m_renderDistance; ++z) {
            glm::ivec3 chunkCoordsToLoad(
                playerChunkCoords.x + x,
                0,
                playerChunkCoords.z + z
            );
            loadChunk(chunkCoordsToLoad);
        }
    }

    unloadFarChunks(playerPosition);

    // Rebuild dirty meshes (usually only on edits)
    for (auto& pair : m_chunks) {
        if (pair.second->isDirty()) {
            pair.second->buildMesh(m_app, m_atlas);
        }
    }

    // NEW: build only a few queued chunks per frame
    processPendingChunks(2);   // tweak this (2–4 is a good start)
}

void ChunkManager::render() const {
    Shader* shader = m_app.getShader();
    if (!shader) return;

    const Frustum& cameraFrustum = m_app.getCamera().getFrustum();

    int renderedChunks = 0;

    for (const auto& pair : m_chunks) {
        glm::vec3 chunkWorldPos = pair.second->getWorldPosition();
        glm::vec3 chunkMax      = chunkWorldPos +
                                  glm::vec3(Chunk::CHUNK_SIZE,
                                            Chunk::CHUNK_HEIGHT,
                                            Chunk::CHUNK_SIZE);

        if (cameraFrustum.containsAABB(chunkWorldPos, chunkMax)) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), chunkWorldPos);
            shader->setMat4("model", model);
            pair.second->render();
            renderedChunks++;
        }
    }

    // Persistent between frames (optional debug)
    static int    lastRenderedChunks = -1;
    static size_t lastTotalChunks    = 0;

    if (renderedChunks != lastRenderedChunks || m_chunks.size() != lastTotalChunks) {
        // ImGui  Log Out
        if (ImGui::Begin("Culling log")) {
            ImGui::TextUnformatted("Culled Chunks");
            ImGui::Separator();
            ImGui::Text("Rendered %d out of %zu chunks.", renderedChunks, m_chunks.size());
        }
        /* Console Log Out
         *std::cout << "Rendered " << renderedChunks
                  << " out of " << m_chunks.size() << " chunks.\n";
        lastRenderedChunks = renderedChunks;
        lastTotalChunks    = m_chunks.size();
        */
        ImGui::End();
    }
}

uint8_t ChunkManager::getBlock(int x, int y, int z) const {
    glm::vec3 worldPos(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
    glm::ivec3 chunkCoords = getChunkCoords(worldPos);
    glm::ivec3 blockLocalCoords = getBlockLocalCoords(worldPos);

    auto it = m_chunks.find(chunkCoords);
    if (it != m_chunks.end()) {
        // Ensure local coordinates are within bounds before accessing
        if (blockLocalCoords.x >= 0 && blockLocalCoords.x < Chunk::CHUNK_SIZE &&
            blockLocalCoords.y >= 0 && blockLocalCoords.y < Chunk::CHUNK_HEIGHT &&
            blockLocalCoords.z >= 0 && blockLocalCoords.z < Chunk::CHUNK_SIZE) {
            return it->second->getBlock(blockLocalCoords.x, blockLocalCoords.y, blockLocalCoords.z);
        }
    }
    return 0; // Return air (or some default empty block) if chunk not loaded or coordinates out of bounds
}

void ChunkManager::processPendingChunks(int maxPerFrame) {
    int processed = 0;
    while (!m_pendingChunks.empty() && processed < maxPerFrame) {
        glm::ivec3 coords = m_pendingChunks.front();
        m_pendingChunks.pop();

        auto it = m_chunks.find(coords);
        if (it == m_chunks.end()) continue; // could have been unloaded

        auto& chunk = *it->second;
        chunk.generate(m_surfaceManager);                    // terrain
        chunk.buildMesh(m_app, m_atlas);     // mesh
        ++processed;
    }
}

void ChunkManager::reloadAllChunks() {
    // Clear the pending queue
    while (!m_pendingChunks.empty()) {
        m_pendingChunks.pop();
    }

    // Regenerate and rebuild all existing chunks
    for (auto& pair : m_chunks) {
        auto& chunk = *pair.second;
        chunk.generate(m_surfaceManager);
        chunk.buildMesh(m_app, m_atlas);
    }
}

