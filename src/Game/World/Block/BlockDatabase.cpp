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
            block.id = blockJson["id"].get<uint8_t>();
            block.name = blockJson["name"].get<std::string>();
            block.isOpaque = blockJson.value("opaque", true);
            block.isSolid = blockJson.value("solid", true);
            block.isTransparent = !block.isOpaque;
            
            // Load textures if present
            if (blockJson.contains("textures") && !blockJson["textures"].is_null()) {
                const auto& texJson = blockJson["textures"];
                
                // Helper to get UV from atlas
                auto getUV = [&atlas](const nlohmann::json& texData) -> UVCoords {
                    if (texData.contains("uv")) {
                        UVCoords uv;
                        uv.min.x = texData["uv"]["min"][0].get<float>();
                        uv.min.y = texData["uv"]["min"][1].get<float>();
                        uv.max.x = texData["uv"]["max"][0].get<float>();
                        uv.max.y = texData["uv"]["max"][1].get<float>();
                        
                        // Flip V for OpenGL
                        uv.min.y = 1.0f - uv.min.y;
                        uv.max.y = 1.0f - uv.max.y;
                        std::swap(uv.min.y, uv.max.y);
                        
                        return uv;
                    }
                    return UVCoords{};
                };
                
                // Check for "all" texture (same on all faces)
                if (texJson.contains("all")) {
                    UVCoords uv = getUV(texJson["all"]);
                    block.textures.topTexture = texJson["all"]["name"].get<std::string>();
                    block.textures.bottomTexture = block.textures.topTexture;
                    block.textures.sideTexture = block.textures.topTexture;
                    
                    for (int i = 0; i < 6; ++i) {
                        block.textures.faceUVs[i] = uv;
                    }
                } else {
                    // Individual face textures
                    if (texJson.contains("top")) {
                        block.textures.topTexture = texJson["top"]["name"].get<std::string>();
                        block.textures.faceUVs[static_cast<size_t>(BlockFace::Top)] = getUV(texJson["top"]);
                    }
                    
                    if (texJson.contains("bottom")) {
                        block.textures.bottomTexture = texJson["bottom"]["name"].get<std::string>();
                        block.textures.faceUVs[static_cast<size_t>(BlockFace::Bottom)] = getUV(texJson["bottom"]);
                    }
                    
                    // Side texture applies to all 4 sides
                    if (texJson.contains("side")) {
                        block.textures.sideTexture = texJson["side"]["name"].get<std::string>();
                        UVCoords sideUV = getUV(texJson["side"]);
                        block.textures.faceUVs[static_cast<size_t>(BlockFace::North)] = sideUV;
                        block.textures.faceUVs[static_cast<size_t>(BlockFace::South)] = sideUV;
                        block.textures.faceUVs[static_cast<size_t>(BlockFace::East)] = sideUV;
                        block.textures.faceUVs[static_cast<size_t>(BlockFace::West)] = sideUV;
                    }
                    
                    // Individual side overrides (e.g., furnace front)
                    if (texJson.contains("front")) {
                        block.textures.southTexture = texJson["front"]["name"].get<std::string>();
                        block.textures.faceUVs[static_cast<size_t>(BlockFace::South)] = getUV(texJson["front"]);
                    }
                }
            }
            
            // Ensure blocks vector is large enough
            if (block.id >= m_blocks.size()) {
                m_blocks.resize(block.id + 1);
            }
            m_blocks[block.id] = std::move(block);
        }
        
        // Setup air block (always ID 0)
        m_airBlock.id = 0;
        m_airBlock.name = "Air";
        m_airBlock.isOpaque = false;
        m_airBlock.isSolid = false;
        m_airBlock.isTransparent = true;
        
        std::cout << "Loaded " << m_blocks.size() << " block definitions" << std::endl;
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

} // namespace MCPP