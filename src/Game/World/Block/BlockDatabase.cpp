#include "Game/World/Block/BlockDatabase.h"

#include <fstream>
#include <iostream>
#include <json.hpp>

namespace MCPP {

BlockDatabase& BlockDatabase::instance() {
    static BlockDatabase instance;
    return instance;
}

bool BlockDatabase::load(const std::string& registryPath, const TextureAtlas& atlas) {
    std::ifstream file(registryPath);
    if (!file.is_open()) {
        std::cerr << "Failed to open block registry: " << registryPath << std::endl;
        return false;
    }

    try {
        nlohmann::json registry;
        file >> registry;

        if (!registry.contains("blocks")) {
            std::cerr << "Invalid registry format: missing 'blocks' array" << std::endl;
            return false;
        }

        m_blocks.clear();

        for (const auto& blockJson : registry["blocks"]) {
            BlockDefinition block;
            block.id           = blockJson["id"].get<uint8_t>();
            block.name         = blockJson["name"].get<std::string>();
            block.isOpaque     = blockJson.value("opaque", true);
            block.isSolid      = blockJson.value("solid",  true);
            block.isTransparent = !block.isOpaque;

            // Default hitbox: full cube for solid, none for non-solid
            if (block.isSolid) {
                block.localHitbox = AABB(glm::vec3(0.0f), glm::vec3(1.0f));
                block.hasHitbox   = true;
            } else {
                block.localHitbox = AABB(glm::vec3(0.0f), glm::vec3(0.0f));
                block.hasHitbox   = false;
            }

            // --- Texture / UV setup ---
            if (blockJson.contains("textures") && !blockJson["textures"].is_null()) {
                const auto& texJson = blockJson["textures"];

                // Helper that pulls UVs from the atlas using texture "name"
                auto getUV = [&atlas](const nlohmann::json& texData) -> UVCoords {
                    if (texData.contains("name")) {
                        const std::string texName = texData["name"].get<std::string>();
                        return atlas.getUV(texName);
                    }
                    return UVCoords{};
                };

                // "all": same texture on all faces
                if (texJson.contains("all")) {
                    UVCoords uvAll = getUV(texJson["all"]);

                    block.textures.topTexture    = texJson["all"]["name"].get<std::string>();
                    block.textures.bottomTexture = block.textures.topTexture;
                    block.textures.sideTexture   = block.textures.topTexture;

                    for (int i = 0; i < 6; ++i) {
                        block.textures.faceUVs[i] = uvAll;
                    }
                } else {
                    // Per-face
                    if (texJson.contains("top")) {
                        block.textures.topTexture =
                            texJson["top"]["name"].get<std::string>();
                        block.textures.faceUVs[static_cast<size_t>(BlockFace::Top)] =
                            getUV(texJson["top"]);
                    }

                    if (texJson.contains("bottom")) {
                        block.textures.bottomTexture =
                            texJson["bottom"]["name"].get<std::string>();
                        block.textures.faceUVs[static_cast<size_t>(BlockFace::Bottom)] =
                            getUV(texJson["bottom"]);
                    }

                    if (texJson.contains("side")) {
                        block.textures.sideTexture =
                            texJson["side"]["name"].get<std::string>();
                        UVCoords sideUV = getUV(texJson["side"]);
                        block.textures.faceUVs[static_cast<size_t>(BlockFace::North)] = sideUV;
                        block.textures.faceUVs[static_cast<size_t>(BlockFace::South)] = sideUV;
                        block.textures.faceUVs[static_cast<size_t>(BlockFace::East)]  = sideUV;
                        block.textures.faceUVs[static_cast<size_t>(BlockFace::West)]  = sideUV;
                    }

                    // Front override (e.g., furnace front)
                    if (texJson.contains("front")) {
                        block.textures.southTexture =
                            texJson["front"]["name"].get<std::string>();
                        block.textures.faceUVs[static_cast<size_t>(BlockFace::South)] =
                            getUV(texJson["front"]);
                    }
                }
            }

            // Ensure blocks vector is large enough
            if (block.id >= m_blocks.size()) {
                m_blocks.resize(block.id + 1);
            }
            m_blocks[block.id] = std::move(block);
        }

        // Air (ID 0)
        m_airBlock.id            = 0;
        m_airBlock.name          = "Air";
        m_airBlock.isOpaque      = false;
        m_airBlock.isSolid       = false;
        m_airBlock.isTransparent = true;
        m_airBlock.localHitbox   = AABB(glm::vec3(0.0f), glm::vec3(0.0f));
        m_airBlock.hasHitbox     = false;

        std::cout << "Loaded " << m_blocks.size() << " block definitions\n";
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error parsing block registry: " << e.what() << std::endl;
        return false;
    }
}

const BlockDefinition& BlockDatabase::getBlock(uint8_t id) const {
    if (id < m_blocks.size()) {
        return m_blocks[id];
    }
    return m_airBlock;
}

const BlockDefinition* BlockDatabase::getBlockByName(const std::string& name) const {
    for (const auto& block : m_blocks) {
        if (block.name == name) {
            return &block;
        }
    }
    return nullptr;
}

UVCoords BlockDatabase::getBlockFaceUV(uint8_t blockId, BlockFace face) const {
    const BlockDefinition& block = getBlock(blockId);
    return block.textures.getUV(face);
}

bool BlockDatabase::isOpaque(uint8_t id) const {
    return getBlock(id).isOpaque;
}

bool BlockDatabase::isSolid(uint8_t id) const {
    return getBlock(id).isSolid;
}

const AABB& BlockDatabase::getLocalHitbox(uint8_t id) const {
    const BlockDefinition& block = getBlock(id);
    return block.localHitbox;
}

} // namespace MCPP
