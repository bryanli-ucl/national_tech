#pragma once

#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

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
    std::unordered_map<std::string, TextureUV> m_texture_uvs; // 纹理名 -> UV坐标
    int m_atlas_size;
    int m_texture_size;
    int m_textures_per_row;

    public:
    TextureAtlas() : m_atlas_size(0), m_texture_size(0), m_textures_per_row(0) {}

    // 从 JSON 文件加载纹理图集元数据
    void loadFromJSON(const std::string& json_path) {
        LOG_INFO("Loading texture atlas from JSON: ", json_path);

        std::ifstream file(json_path);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open JSON file: " + json_path);
        }

        nlohmann::json j;
        try {
            file >> j;
        } catch (const nlohmann::json::exception& e) {
            throw std::runtime_error("Failed to parse JSON: " + std::string(e.what()));
        }

        // 读取基本信息
        m_texture_size     = j["texture_size"].get<int>();
        m_atlas_size       = j["atlas_size"].get<int>();
        m_textures_per_row = j["textures_per_row"].get<int>();

        LOG_DEBUG("Atlas metadata:");
        LOG_DEBUG("  Atlas size: ", m_atlas_size, "x", m_atlas_size);
        LOG_DEBUG("  Texture size: ", m_texture_size, "x", m_texture_size);
        LOG_DEBUG("  Textures per row: ", m_textures_per_row);

        // 读取所有纹理
        const auto& textures = j["textures"];
        LOG_DEBUG("Loading ", textures.size(), " textures:");

        for (auto& [name, data] : textures.items()) {
            // 读取 UV 坐标
            float min_u = data["uv"]["min"][0].get<float>();
            float min_v = data["uv"]["min"][1].get<float>();
            float max_u = data["uv"]["max"][0].get<float>();
            float max_v = data["uv"]["max"][1].get<float>();

            m_texture_uvs[name] = TextureUV(min_u, min_v, max_u, max_v);

            int index = data["index"].get<int>();
            LOG_DEBUG("  [", index, "] '", name, "' UV: (", min_u, ",", min_v, ") -> (", max_u, ",", max_v, ")");
        }


        LOG_DEBUG("Atlas setup:");
        LOG_DEBUG("  Atlas size: ", m_atlas_size, "x", m_atlas_size);
        LOG_DEBUG("  Texture size: ", m_texture_size, "x", m_texture_size);
        LOG_DEBUG("  Textures per row: ", m_textures_per_row);
        LOG_DEBUG("  Max textures: ", m_textures_per_row * (m_atlas_size / m_texture_size));

        LOG_INFO("Successfully loaded ", m_texture_uvs.size(), " textures from atlas");
    }

    TextureUV getUV(const std::string& name) const {
        auto it = m_texture_uvs.find(name);
        if (it != m_texture_uvs.end()) {
            return it->second;
        }

        LOG_WARN("Warning: Texture '", name, "' not found, using default");

        if (!m_texture_uvs.empty()) {
            return m_texture_uvs.begin()->second;
        }
        return TextureUV();
    }

    bool hasTexture(const std::string& name) const {
        return m_texture_uvs.find(name) != m_texture_uvs.end();
    }

    size_t getTextureCount() const {
        return m_texture_uvs.size();
    }

    // 获取图集信息
    int getAtlasSize() const { return m_atlas_size; }
    int getTextureSize() const { return m_texture_size; }
    int getTexturesPerRow() const { return m_textures_per_row; }
};

} // namespace renderer