#include "Game/Rendering/TextureAtlas.h"

#include <fstream>
#include <iostream>
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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
        m_width     = other.m_width;
        m_height    = other.m_height;
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
        std::cerr << "Warning: Failed to load atlas mapping, UVs will be default\n";
        // We still keep the texture; rendering will just use default UV (0..1).
    }

    return true;
}

bool TextureAtlas::loadImage(const std::string& path) {
    stbi_set_flip_vertically_on_load(true);

    int channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &m_width, &m_height, &channels, 4);

    if (!data) {
        std::cerr << "Failed to load texture atlas: " << path << "\n";
        std::cerr << "STB Error: " << stbi_failure_reason() << "\n";
        return false;
    }

    std::cout << "Loaded atlas: " << path << " (" << m_width << "x" << m_height << ")\n";

    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Pixel-perfect for block textures; this avoids interpolation bleed.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 m_width, m_height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);

    // Mipmaps are generated but with MIN_FILTER=NEAREST only level 0 is used.
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return true;
}

bool TextureAtlas::loadMapping(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open mapping file: " << path << "\n";
        return false;
    }

    try {
        nlohmann::json mapping;
        file >> mapping;

        for (auto& [texName, texData] : mapping.items()) {
            UVCoords uv;

            if (texData.contains("pixel")) {
                auto& pixel = texData["pixel"];
                int px = pixel[0].get<int>(); // pixel x in atlas
                int py = pixel[1].get<int>(); // pixel y in atlas

                constexpr int TILE_SIZE = 16;

                float texelU = 1.0f / static_cast<float>(m_width);   // 1/4096
                float texelV = 1.0f / static_cast<float>(m_height);  // 1/4096

                // Half-texel inset to avoid touching neighbour tiles.
                float insetU = texelU * 0.5f;
                float insetV = texelV * 0.5f;

                // NOTE: correct formula is (px * texel + inset), not (px + inset)*texel
                float uMin = px * texelU + insetU;
                float vMin = py * texelV + insetV;
                float uMax = (px + TILE_SIZE) * texelU - insetU;
                float vMax = (py + TILE_SIZE) * texelV - insetV;

                uv.min.x = uMin;
                uv.min.y = vMin;
                uv.max.x = uMax;
                uv.max.y = vMax;

                // Flip V because image data was flipped on load for OpenGL
                uv.min.y = 1.0f - uv.min.y;
                uv.max.y = 1.0f - uv.max.y;
                std::swap(uv.min.y, uv.max.y);
            }

            m_uvMapping[texName] = uv;
        }

        std::cout << "Loaded " << m_uvMapping.size() << " texture mappings\n";
        return true;

    } catch (const std::exception& e) {
        std::cerr << "JSON parsing error in atlas mapping: " << e.what() << "\n";
        return false;
    }
}

void TextureAtlas::bind(unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
}

void TextureAtlas::unbind() {
    glBindTexture(GL_TEXTURE_2D, 0);
}

UVCoords TextureAtlas::getUV(const std::string& textureName) const {
    auto it = m_uvMapping.find(textureName);
    if (it != m_uvMapping.end()) {
        return it->second;
    }
    return UVCoords{}; // fallback: full texture
}

bool TextureAtlas::hasTexture(const std::string& textureName) const {
    return m_uvMapping.contains(textureName);
}

} // namespace MCPP
