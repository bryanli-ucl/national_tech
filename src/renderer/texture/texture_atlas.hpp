#pragma once

#include <glm/glm.hpp>
#include <iostream>
#include <string>
#include <unordered_map>

#include "utils/logger.hpp"

namespace renderer {

struct TextureUV {
    glm::vec2 min;
    glm::vec2 max;

    TextureUV() : min(0.0f, 0.0f), max(1.0f, 1.0f) {}
    TextureUV(float minU, float minV, float maxU, float maxV)
    : min(minU, minV), max(maxU, maxV) {}
};

class TextureAtlas {
    private:
    std::unordered_map<std::string, int> m_texture_indices; // 纹理名 -> 索引
    int m_atlas_size;
    int m_texture_size;
    int m_textures_per_row;

    public:
    TextureAtlas() : m_atlas_size(0), m_texture_size(0), m_textures_per_row(0) {}

    // 设置图集信息（从图片尺寸自动计算）
    void setupFromImageSize(int atlas_width, int atlas_height, int texture_size) {
        m_atlas_size       = atlas_width; // 假设是正方形，使用宽度
        m_texture_size     = texture_size;
        m_textures_per_row = atlas_width / texture_size;

        LOG_DEBUG("Atlas setup:");
        LOG_DEBUG("  Atlas size: ", m_atlas_size, "x", atlas_height);
        LOG_DEBUG("  Texture size: ", m_texture_size, "x", m_texture_size);
        LOG_DEBUG("  Textures per row: ", m_textures_per_row);
        LOG_DEBUG("  Max textures: ", m_textures_per_row * (atlas_height / texture_size));
    }

    // 手动注册纹理名称和索引
    void registerTexture(const std::string& name, int index) {
        m_texture_indices[name] = index;
        LOG_DEBUG("  Registered: '", name, "' at index ", index);
    }

    // 根据索引计算 UV
    TextureUV getUVByIndex(int index) const {
        if (m_textures_per_row == 0) {
            LOG_ERROR("Atlas not initialized!");
            return TextureUV();
        }

        int row = index / m_textures_per_row;
        int col = index % m_textures_per_row;

        float u_min = (float)col / m_textures_per_row;
        float v_min = (float)row / m_textures_per_row;
        float u_max = (float)(col + 1) / m_textures_per_row;
        float v_max = (float)(row + 1) / m_textures_per_row;

        return TextureUV(u_min, v_min, u_max, v_max);
    }

    // 根据名称获取 UV
    TextureUV getUV(const std::string& name) const {
        auto it = m_texture_indices.find(name);
        if (it != m_texture_indices.end()) {
            return getUVByIndex(it->second);
        }

        LOG_WARN("Warning: Texture '", name, "' not found, using default");
        return getUVByIndex(0); // 返回第一个纹理
    }

    bool hasTexture(const std::string& name) const {
        return m_texture_indices.find(name) != m_texture_indices.end();
    }

    size_t getTextureCount() const {
        return m_texture_indices.size();
    }
};

} // namespace renderer