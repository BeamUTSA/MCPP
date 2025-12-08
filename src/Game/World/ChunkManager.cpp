#include "Game/World/ChunkManager.h"
#include "Game/MinecraftApp.h"
#include "Game/Rendering/TextureAtlas.h"
#include "Game/World/Meshing/Greedy.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace Meshing;

ChunkManager::ChunkManager(MinecraftApp& app, MCPP::TextureAtlas& atlas)
    : m_app(app), m_atlas(atlas), m_surfaceManager(1337u)
{
    const unsigned int threads = std::max(2u, std::thread::hardware_concurrency());

    // 2 generation threads + (threads-2) meshing threads
    for (unsigned int i = 0; i < 2; ++i)
        m_generationWorkers.emplace_back(&ChunkManager::generationWorker, this);

    for (unsigned int i = 0; i < threads - 2; ++i)
        m_meshingWorkers.emplace_back(&ChunkManager::meshingWorker, this);
}

ChunkManager::~ChunkManager() {
    m_shutdown = true;
    m_taskCV.notify_all();

    for (auto& t : m_generationWorkers) if (t.joinable()) t.join();
    for (auto& t : m_meshingWorkers)    if (t.joinable()) t.join();
}

void ChunkManager::generationWorker() {
    while (!m_shutdown) {
        glm::ivec3 coords;
        {
            std::unique_lock<std::mutex> lock(m_taskMutex);
            m_taskCV.wait(lock, [this] { return !m_generationQueue.empty() || m_shutdown; });
            if (m_shutdown) break;
            if (m_generationQueue.empty()) continue;

            coords = m_generationQueue.front();
            m_generationQueue.pop();
        }

        // Thread-safe chunk access with m_chunkMutex
        Chunk* chunkPtr = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_chunkMutex);
            auto it = m_chunks.find(coords);
            if (it == m_chunks.end()) continue;
            chunkPtr = it->second.get();
        }

        // Generate terrain outside the lock (safe - only modifies chunk's internal data)
        chunkPtr->generate(m_surfaceManager);

        {
            std::lock_guard<std::mutex> lock(m_taskMutex);
            m_meshingQueue.push(coords);
        }
        m_taskCV.notify_one();
    }
}

void ChunkManager::meshingWorker() {
    while (!m_shutdown) {
        glm::ivec3 coords;
        {
            std::unique_lock<std::mutex> lock(m_taskMutex);
            m_taskCV.wait(lock, [this] { return !m_meshingQueue.empty() || m_shutdown; });
            if (m_shutdown) break;
            if (m_meshingQueue.empty()) continue;

            coords = m_meshingQueue.front();
            m_meshingQueue.pop();
        }

        // Thread-safe chunk access with m_chunkMutex
        const Chunk* chunkPtr = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_chunkMutex);
            auto it = m_chunks.find(coords);
            if (it == m_chunks.end()) continue;
            chunkPtr = it->second.get();
        }

        // Build mesh outside the lock (read-only access to chunk data)
        std::vector<ChunkVertex> vertices;
        vertices.reserve(40 * 1024);
        buildGreedyMesh(*chunkPtr, m_app, vertices);

        {
            std::lock_guard<std::mutex> lock(m_taskMutex);
            m_uploadQueue.push({coords, std::move(vertices)});
        }
    }
}

void ChunkManager::loadChunk(const glm::ivec3& chunkCoords) {
    // Check if chunk exists (with chunk mutex)
    {
        std::lock_guard<std::mutex> lock(m_chunkMutex);
        if (m_chunks.contains(chunkCoords)) return;
    }

    // Add to map and queue (need both locks, acquire in consistent order to avoid deadlock)
    std::lock_guard<std::mutex> chunkLock(m_chunkMutex);
    std::lock_guard<std::mutex> taskLock(m_taskMutex);

    // Double-check after acquiring locks
    if (m_chunks.contains(chunkCoords)) return;

    auto chunk = std::make_unique<Chunk>(chunkCoords);
    m_chunks[chunkCoords] = std::move(chunk);
    m_generationQueue.push(chunkCoords);
    m_taskCV.notify_one();
}

void ChunkManager::update(const glm::vec3& playerPosition) {
    constexpr int renderDistance = 24;  // or make it a member
    const glm::ivec3 playerChunk = getChunkCoords(playerPosition);

    std::vector<glm::ivec3> toLoad;
    std::vector<glm::ivec3> toUnload;

    // === LOAD NEW CHUNKS ===
    {
        std::lock_guard<std::mutex> lock(m_chunkMutex);
        for (int x = -renderDistance; x <= renderDistance; ++x) {
            for (int z = -renderDistance; z <= renderDistance; ++z) {
                glm::ivec3 coords(playerChunk.x + x, 0, playerChunk.z + z);
                float dist = glm::distance(glm::vec2(coords.x, coords.z), glm::vec2(playerChunk.x, playerChunk.z));

                if (dist <= renderDistance) {
                    if (m_chunks.find(coords) == m_chunks.end()) {
                        toLoad.push_back(coords);
                    }
                }
            }
        }
    }

    // === UNLOAD FAR CHUNKS ===
    {
        std::lock_guard<std::mutex> lock(m_chunkMutex);
        for (auto it = m_chunks.begin(); it != m_chunks.end(); ) {
            float dist = glm::distance(glm::vec2(it->first.x, it->first.z), glm::vec2(playerChunk.x, playerChunk.z));
            if (dist > renderDistance + 2) {
                toUnload.push_back(it->first);
                it = m_chunks.erase(it);
            } else {
                ++it;
            }
        }
    }

    // === QUEUE NEW CHUNKS FOR GENERATION ===
    if (!toLoad.empty()) {
        std::lock_guard<std::mutex> chunkLock(m_chunkMutex);
        std::lock_guard<std::mutex> taskLock(m_taskMutex);
        for (auto& coords : toLoad) {
            // Double-check chunk doesn't exist (could have been added by another thread)
            if (m_chunks.find(coords) == m_chunks.end()) {
                auto chunk = std::make_unique<Chunk>(coords);
                m_chunks[coords] = std::move(chunk);
                m_generationQueue.push(coords);
            }
        }
        m_taskCV.notify_all();
    }

    // === UPLOAD FINISHED MESHES (main thread only) ===
    const int MAX_UPLOAD_PER_FRAME = 16;
    for (int i = 0; i < MAX_UPLOAD_PER_FRAME && !m_uploadQueue.empty(); ) {
        MeshTask task;
        {
            std::lock_guard<std::mutex> lock(m_taskMutex);
            if (m_uploadQueue.empty()) break;
            task = std::move(m_uploadQueue.front());
            m_uploadQueue.pop();
        }

        {
            std::lock_guard<std::mutex> lock(m_chunkMutex);
            auto it = m_chunks.find(task.chunkCoords);
            if (it != m_chunks.end()) {
                it->second->uploadMesh(task.vertices);
                it->second->markDirty(false);
                ++i;
            }
        }
    }
}

void ChunkManager::reloadAllChunks() {
    std::lock_guard<std::mutex> chunkLock(m_chunkMutex);
    std::lock_guard<std::mutex> taskLock(m_taskMutex);

    // Clear queues
    std::queue<glm::ivec3> emptyQ; std::swap(m_generationQueue, emptyQ);
    std::queue<glm::ivec3> emptyM; std::swap(m_meshingQueue, emptyM);
    std::queue<MeshTask>   emptyU; std::swap(m_uploadQueue, emptyU);

    // Re-queue all chunks
    for (auto& pair : m_chunks) {
        pair.second->markDirty(false);
        m_generationQueue.push(pair.first);
    }

    m_taskCV.notify_all();
    std::cout << "[ChunkManager] Reloaded " << m_chunks.size() << " chunks (multithreaded, instant)\n";
}

// ============================================================================
// RENDER ALL VISIBLE CHUNKS
// ============================================================================
void ChunkManager::render() const {
    std::lock_guard<std::mutex> lock(m_chunkMutex);
    for (const auto& pair : m_chunks) {
        const auto& chunk = pair.second;
        if (chunk && chunk->hasGeometry()) {
            chunk->render();
        }
    }
}

// ============================================================================
// BLOCK QUERY — used by player, raycasting, etc.
// ============================================================================
uint8_t ChunkManager::getBlock(int x, int y, int z) const {
    const glm::ivec3 worldPos(x, y, z);
    const glm::ivec3 chunkCoords = getChunkCoords(glm::vec3(worldPos));
    const glm::ivec3 local = getBlockLocalCoords(glm::vec3(worldPos));

    std::lock_guard<std::mutex> lock(m_chunkMutex);
    auto it = m_chunks.find(chunkCoords);
    if (it == m_chunks.end()) {
        return 0; // air
    }

    if (local.y < 0 || local.y >= Chunk::CHUNK_HEIGHT) {
        return 0;
    }

    return it->second->getBlock(local.x, local.y, local.z);
}

// ============================================================================
// CHUNK COORDINATES FROM WORLD POSITION
// ============================================================================
glm::ivec3 ChunkManager::getChunkCoords(const glm::vec3& worldPos) const {
    return glm::ivec3(
        static_cast<int>(std::floor(worldPos.x / Chunk::CHUNK_SIZE)),
        0,
        static_cast<int>(std::floor(worldPos.z / Chunk::CHUNK_SIZE))
    );
}

// ============================================================================
// LOCAL BLOCK COORDINATES INSIDE A CHUNK
// ============================================================================
glm::ivec3 ChunkManager::getBlockLocalCoords(const glm::vec3& worldPos) const {
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