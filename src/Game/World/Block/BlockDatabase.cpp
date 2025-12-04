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

            if (block.isSolid) {
                block.localHitbox = AABB(glm::vec3(0.0f), glm::vec3(1.0f));
                block.hasHitbox   = true;
            } else {
                block.localHitbox = AABB(glm::vec3(0.0f), glm::vec3(0.0f));
                block.hasHitbox   = false;
            }

            if (blockJson.contains("textures") && !blockJson["textures"].is_null()) {
                const auto& texJson = blockJson["textures"];

                auto getUVFromObject = [&atlas](const nlohmann::json& texObject) -> UVCoords {
                    if (texObject.is_object() && texObject.contains("name")) {
                        return atlas.getUV(texObject["name"].get<std::string>());
                    }
                    return UVCoords{};
                };
                
                auto getNameFromObject = [](const nlohmann::json& texObject) -> std::string {
                    if (texObject.is_object() && texObject.contains("name")) {
                        return texObject["name"].get<std::string>();
                    }
                    return "";
                };

                if (texJson.contains("all")) {
                    const auto& allTexObject = texJson["all"];
                    std::string texName = getNameFromObject(allTexObject);
                    UVCoords uvAll = getUVFromObject(allTexObject);
                    
                    block.textures.topTexture    = texName;
                    block.textures.bottomTexture = texName;
                    block.textures.sideTexture   = texName;
                    for (int i = 0; i < 6; ++i) {
                        block.textures.faceUVs[i] = uvAll;
                    }
                } else {
                    if (texJson.contains("top")) {
                        const auto& texObject = texJson["top"];
                        block.textures.topTexture = getNameFromObject(texObject);
                        block.textures.faceUVs[static_cast<size_t>(BlockFace::Top)] = getUVFromObject(texObject);
                    }
                    if (texJson.contains("bottom")) {
                        const auto& texObject = texJson["bottom"];
                        block.textures.bottomTexture = getNameFromObject(texObject);
                        block.textures.faceUVs[static_cast<size_t>(BlockFace::Bottom)] = getUVFromObject(texObject);
                    }
                    if (texJson.contains("side")) {
                        const auto& texObject = texJson["side"];
                        block.textures.sideTexture = getNameFromObject(texObject);
                        UVCoords sideUV = getUVFromObject(texObject);
                        block.textures.faceUVs[static_cast<size_t>(BlockFace::North)] = sideUV;
                        block.textures.faceUVs[static_cast<size_t>(BlockFace::South)] = sideUV;
                        block.textures.faceUVs[static_cast<size_t>(BlockFace::East)]  = sideUV;
                        block.textures.faceUVs[static_cast<size_t>(BlockFace::West)]  = sideUV;
                    }
                    if (texJson.contains("front")) {
                        const auto& texObject = texJson["front"];
                        block.textures.southTexture = getNameFromObject(texObject);
                        block.textures.faceUVs[static_cast<size_t>(BlockFace::South)] = getUVFromObject(texObject);
                    }
                }
            }

            if (block.id >= m_blocks.size()) {
                m_blocks.resize(block.id + 1);
            }
            m_blocks[block.id] = std::move(block);
        }

        m_airBlock.id = 0;
        m_airBlock.name = "Air";
        m_airBlock.isOpaque = false;
        m_airBlock.isSolid = false;
        m_airBlock.isTransparent = true;
        m_airBlock.hasHitbox = false;

        std::cout << "Loaded " << m_blocks.size() << " block definitions\n";
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error parsing block registry: " << e.what() << std::endl;
        return false;
    }
}

const BlockDefinition& BlockDatabase::getBlock(uint8_t id) const {
    if (id < m_blocks.size()) {
        // Ensure the ID is valid and the block data is loaded, otherwise return air
        if (!m_blocks[id].name.empty()) {
            return m_blocks[id];
        }
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