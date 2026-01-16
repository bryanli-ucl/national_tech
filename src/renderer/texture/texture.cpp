#include "texture.hpp"


#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <iostream>
#include <stdexcept>

#include "utils/logger.hpp"

namespace renderer {

texture::texture(const std::string& path, bool flip)
: m_id(0), m_width(0), m_height(0), m_channels(0) {

    stbi_set_flip_vertically_on_load(flip);

    unsigned char* data = stbi_load(path.c_str(), &m_width, &m_height, &m_channels, 0);

    if (!data) {
        throw std::runtime_error("Failed to load texture: " + path);
    }

    LOG_DEBUG("Loaded texture: ", path, " (", m_width, "x", m_height, ", ", m_channels, " channels)");

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    // 设置纹理参数 - MC 风格使用最近邻过滤
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLenum format = GL_RGB;
    if (m_channels == 1)
        format = GL_RED;
    else if (m_channels == 3)
        format = GL_RGB;
    else if (m_channels == 4)
        format = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, format, m_width, m_height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

texture::~texture() {
    if (m_id != 0) {
        glDeleteTextures(1, &m_id);
    }
}

auto texture::bind(uint32_t slot) -> void {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_id);
}

auto texture::unbind() -> void {
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace renderer