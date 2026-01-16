#pragma once

#include <glad/glad.h>
#include <string>

namespace renderer {
class texture {
    private:
    uint32_t m_id;

    int m_width;
    int m_height;
    int m_channels;

    public:
    texture(const std::string& path, bool flip = true);
    ~texture();

    auto bind(uint32_t slot = 0) -> void;
    auto unbind() -> void;

    auto get_id() const -> uint32_t { return m_id; }
    auto get_width() const -> int { return m_width; }
    auto get_height() const -> int { return m_height; }
};


} // namespace renderer
