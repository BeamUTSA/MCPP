#pragma once

#include "Game/Rendering/TextureAtlas.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace MCPP {

// Face indices for texture lookups
enum class BlockFace : uint8_t {
    Top = 0,
    Bottom = 1,
    North = 2,   // -Z
    South = 3,   // +Z
    East = 4,    // +X
    West = 5,    // -X
    COUNT = 6
};

struct BlockTextures {
    // UV coordinates for each face
    std::array<UVCoords, 6> faceUVs;
    
    // Texture names for reference
    std::string topTexture;
    std::string bottomTexture;
    std::string sideTexture;  // Used for all 4 sides if individual not specified
    std::string northTexture;
    std::string southTexture;
    std::string eastTexture;
    std::string westTexture;
    
    const UVCoords& getUV(BlockFace face) const {
        return faceUVs[static_cast<size_t>(face)];
    }
};

struct BlockDefinition {
    uint8_t id{0};
    std::string name;
    bool isOpaque{true};      // Does this block block light?
    bool isSolid{true};       // Does this block have collision?
    bool isTransparent{false}; // Should this block be rendered in transparent pass?
    
    BlockTextures textures;
};

class BlockDatabase {
public:
    static BlockDatabase& instance();
    
    /**
     * Initialize the database from registry JSON and texture atlas
     * @param registryPath Path to block_registry.json
     * @param atlas Reference to loaded texture atlas
     */
    bool load(const std::string& registryPath, const TextureAtlas& atlas);
    
    /**
     * Get block definition by ID
     */
    const BlockDefinition& getBlock(uint8_t id) const;
    
    /**
     * Get block definition by name
     */
    const BlockDefinition* getBlockByName(const std::string& name) const;
    
    /**
     * Get UV coordinates for a specific block face
     */
    UVCoords getBlockFaceUV(uint8_t blockId, BlockFace face) const;
    
    /**
     * Check if a block is opaque (blocks light and vision)
     */
    bool isOpaque(uint8_t id) const;
    
    /**
     * Check if a block is solid (has collision)
     */
    bool isSolid(uint8_t id) const;
    
    /**
     * Check if a block is air (ID 0)
     */
    static bool isAir(uint8_t id) { return id == 0; }
    
    /**
     * Get total number of registered blocks
     */
    size_t getBlockCount() const { return m_blocks.size(); }
    
private:
    BlockDatabase() = default;
    
    std::vector<BlockDefinition> m_blocks;
    BlockDefinition m_airBlock; // Default for invalid IDs
};

} // namespace MCPP