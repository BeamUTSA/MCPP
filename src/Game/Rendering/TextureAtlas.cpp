#include "Game/Rendering/TextureAtlas.h"

#include <fstream>
#include <iostream>
#include <sstream>

// Define STB_IMAGE_IMPLEMENTATION in exactly one .cpp file
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Simple JSON parsing (you can replace with nlohmann/json for more robust parsing)
#include <json.hpp>

namespace MCPP {

TextureAtlas::~TextureAtlas() {
    if (m_textureID != 0) {
        glDeleteTextures(1, &m_textureID);
    }
}

TextureAtlas::TextureAtlas(TextureAtlas&& other) noexcept
    : m_textureID(other.m_textureID)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_uvMapping(std::move(other.m_uvMapping)) {
    other.m_textureID = 0;
}

TextureAtlas& TextureAtlas::operator=(TextureAtlas&& other) noexcept {
    if (this != &other) {
        if (m_textureID != 0) {
            glDeleteTextures(1, &m_textureID);
        }
        m_textureID = other.m_textureID;
        m_width = other.m_width;
        m_height = other.m_height;
        m_uvMapping = std::move(other.m_uvMapping);
        other.m_textureID = 0;
    }
    return *this;
}

bool TextureAtlas::load(const std::string& atlasPath, const std::string& mappingPath) {
    if (!loadImage(atlasPath)) {
        return false;
    }
    
    if (!loadMapping(mappingPath)) {
        std::cerr << "Warning: Failed to load atlas mapping, UVs will be default" << std::endl;
        // Continue anyway - the texture is loaded
    }
    
    return true;
}

bool TextureAtlas::loadImage(const std::string& path) {
    // Flip vertically for OpenGL (bottom-left origin)
    stbi_set_flip_vertically_on_load(true);
    
    int channels;
    unsigned char* data = stbi_load(path.c_str(), &m_width, &m_height, &channels, 4);
    
    if (!data) {
        std::cerr << "Failed to load texture atlas: " << path << std::endl;
        std::cerr << "STB Error: " << stbi_failure_reason() << std::endl;
        return false;
    }
    
    std::cout << "Loaded atlas: " << path << " (" << m_width << "x" << m_height << ")" << std::endl;
    
    // Generate OpenGL texture
    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    
    // Set texture parameters for pixel art (no filtering, nearest neighbor)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    
    // Generate mipmaps for better quality at distance
    glGenerateMipmap(GL_TEXTURE_2D);
    
    // Set max anisotropic filtering if available
    float maxAniso = 1.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, std::min(maxAniso, 4.0f));
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    stbi_image_free(data);
    return true;
}

bool TextureAtlas::loadMapping(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open mapping file: " << path << std::endl;
        return false;
    }
    
    try {
        nlohmann::json mapping;
        file >> mapping;
        
        for (auto& [texName, texData] : mapping.items()) {
            UVCoords uv;
            
            if (texData.contains("uv")) {
                auto& uvData = texData["uv"];
                uv.min.x = uvData["min"][0].get<float>();
                uv.min.y = uvData["min"][1].get<float>();
                uv.max.x = uvData["max"][0].get<float>();
                uv.max.y = uvData["max"][1].get<float>();
                
                // Flip V coordinates for OpenGL (we flipped the image on load)
                uv.min.y = 1.0f - uv.min.y;
                uv.max.y = 1.0f - uv.max.y;
                std::swap(uv.min.y, uv.max.y);
            }
            
            m_uvMapping[texName] = uv;
        }
        
        std::cout << "Loaded " << m_uvMapping.size() << " texture mappings" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        return false;
    }
}

void TextureAtlas::bind(unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
}

void TextureAtlas::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

UVCoords TextureAtlas::getUV(const std::string& textureName) const {
    auto it = m_uvMapping.find(textureName);
    if (it != m_uvMapping.end()) {
        return it->second;
    }
    return UVCoords{}; // Default full texture
}

bool TextureAtlas::hasTexture(const std::string& textureName) const {
    return m_uvMapping.find(textureName) != m_uvMapping.end();
}

} // namespace MCPP