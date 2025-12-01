#pragma once

#include <string>
#include <cstdint>

enum class BlockID : uint8_t {
    Air = 0,
    Stone,
    Dirt,
    Grass,
    // More blocks will be added here
};

struct BlockData {
    BlockID id;
    std::string name;
    bool isOpaque;
    
    // Texture file names for each face
    std::string topTexture;
    std::string bottomTexture;
    std::string sideTexture;
};
