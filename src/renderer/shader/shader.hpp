#pragma once

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>


#include "utils/file.hpp"


namespace renderer {

class shader {

    uint32_t m_id;

    enum class shader_type_t : uint32_t {
        PROGRAM,
        VERTEX,
        FRAGMENT,
    };

    public:
    shader(std::string vertex_shader_path, std::string fragment_shader_path);
    ~shader();

    auto activate() -> void;

    template <typename T>
    auto set(const std::string& name, const T& val) {

        using BT = std::remove_cv_t<std::remove_reference_t<T>>;

        GLint location = glGetUniformLocation(m_id, name.c_str());

        if (location == -1) {
            // throw std::runtime_error("cannot find variable: " + name);
            return;
        }

        // scalar
        if constexpr (std::is_integral_v<BT>) {
            glUniform1i(location, val);
        }
        if constexpr (std::is_floating_point_v<BT>) {
            glUniform1f(location, val);
        }
        // matrix
        if constexpr (std::is_same_v<BT, glm::mat4>) {
            glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(val));
        }
        if constexpr (std::is_same_v<BT, glm::mat3>) {
            glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(val));
        }
        if constexpr (std::is_same_v<BT, glm::mat2>) {
            glUniformMatrix2fv(location, 1, GL_FALSE, glm::value_ptr(val));
        }
        // vector
        if constexpr (std::is_same_v<BT, glm::vec4>) {
            glUniform4fv(location, 1, glm::value_ptr(val));
        }
        if constexpr (std::is_same_v<BT, glm::vec3>) {
            glUniform3fv(location, 1, glm::value_ptr(val));
        }
        if constexpr (std::is_same_v<BT, glm::vec2>) {
            glUniform2fv(location, 1, glm::value_ptr(val));
        }
    }

    auto get_id() -> uint32_t { return m_id; }

    private:
    auto check_compile_error(uint32_t id, const shader_type_t type) -> void;
};
} // namespace renderer
