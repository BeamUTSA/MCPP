#pragma once

#include "Game/Rendering/TextureAtlas.h"
#include "Engine/Physics/Collision.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace MCPP {

// Face indices for texture lookups
// Top, Bottom, North(Z-), South(Z+), East(X+), West(X-)
enum class BlockFace : uint8_t {
    Top    = 0,
    Bottom = 1,
    North  = 2,
    South  = 3,
    East   = 4,
    West   = 5
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
    uint8_t id{1};
    std::string name;
    bool isOpaque{true};       // Does this block block light?
    bool isSolid{true};        // Does this block have collision?
    bool isTransparent{false}; // Should this block be rendered in transparent pass?

    // Local-space collision box, in block coordinates.
    // Default is a full cube [0,0,0] -> [1,1,1] for solid blocks.
    // For non-solid blocks (air, flowers) this will be empty and hasHitbox=false.
    AABB localHitbox;
    bool hasHitbox{false};

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

    const AABB& getLocalHitbox(uint8_t id) const;

    /**
     * Check if a block actually has a collision hitbox.
     */
    bool hasHitbox(uint8_t id) const { return getBlock(id).hasHitbox; }
    
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