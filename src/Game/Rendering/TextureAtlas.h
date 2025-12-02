#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

namespace MCPP {

struct UVCoords {
    glm::vec2 min{0.0f};  // Bottom-left UV
    glm::vec2 max{1.0f};  // Top-right UV
    
    // Get UV for each corner of a face
    glm::vec2 bottomLeft() const { return min; }
    glm::vec2 bottomRight() const { return {max.x, min.y}; }
    glm::vec2 topRight() const { return max; }
    glm::vec2 topLeft() const { return {min.x, max.y}; }
};

class TextureAtlas {
public:
    TextureAtlas() = default;
    ~TextureAtlas();
    
    // Non-copyable
    TextureAtlas(const TextureAtlas&) = delete;
    TextureAtlas& operator=(const TextureAtlas&) = delete;
    
    // Movable
    TextureAtlas(TextureAtlas&& other) noexcept;
    TextureAtlas& operator=(TextureAtlas&& other) noexcept;
    
    /**
     * Load atlas from PNG file and mapping JSON
     * @param atlasPath Path to the atlas PNG
     * @param mappingPath Path to the atlas_mapping.json
     * @return true on success
     */
    bool load(const std::string& atlasPath, const std::string& mappingPath);
    
    /**
     * Bind the atlas texture to a texture unit
     * @param unit Texture unit (0-15)
     */
    void bind(unsigned int unit = 0) const;
    
    /**
     * Unbind the texture
     */
    void unbind() const;
    
    /**
     * Get UV coordinates for a texture by name
     * @param textureName Name of the texture (e.g., "stone", "dirt")
     * @return UV coordinates, or default (0,0)-(1,1) if not found
     */
    UVCoords getUV(const std::string& textureName) const;
    
    /**
     * Check if a texture exists in the atlas
     */
    bool hasTexture(const std::string& textureName) const;
    
    /**
     * Get the OpenGL texture ID
     */
    GLuint getTextureID() const { return m_textureID; }
    
    /**
     * Get atlas dimensions
     */
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    
private:
    bool loadImage(const std::string& path);
    bool loadMapping(const std::string& path);
    
    GLuint m_textureID{0};
    int m_width{0};
    int m_height{0};
    
    std::unordered_map<std::string, UVCoords> m_uvMapping;
};

} // namespace MCPP