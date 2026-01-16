#include "shader.hpp"

namespace renderer {

shader::shader(std::string vertex_shader_path, std::string fragment_shader_path) {
    using std::string;

    string vertex_code   = utils::read_file(vertex_shader_path);
    string fragment_code = utils::read_file(fragment_shader_path);

    const char* vertex_code_c   = vertex_code.c_str();
    const char* fragment_code_c = fragment_code.c_str();

    uint32_t vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertex_code_c, nullptr);
    glCompileShader(vertex);
    check_compile_error(vertex, shader_type_t::VERTEX);

    uint32_t fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragment_code_c, nullptr);
    glCompileShader(fragment);
    check_compile_error(fragment, shader_type_t::FRAGMENT);

    m_id = glCreateProgram();
    glAttachShader(m_id, vertex);
    glAttachShader(m_id, fragment);
    glLinkProgram(m_id);
    check_compile_error(m_id, shader_type_t::PROGRAM);

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}


shader::~shader() {
    if (m_id != 0 && m_id != static_cast<uint32_t>(-1)) {
        glDeleteProgram(m_id);
    }
    m_id = 0;
}

auto shader::activate() -> void {
    glUseProgram(m_id);
}

auto shader::check_compile_error(uint32_t id, const shader_type_t type) -> void {
    int ret;
    char info[256];
    if (type == shader_type_t::PROGRAM) {

        glGetProgramiv(id, GL_LINK_STATUS, &ret);

        if (ret == GL_FALSE) {
            glGetProgramInfoLog(id, 1024, nullptr, info);
            throw std::runtime_error(info);
        }

    } else if (type == shader_type_t::VERTEX || type == shader_type_t::FRAGMENT) {

        glGetShaderiv(id, GL_COMPILE_STATUS, &ret);

        if (ret == GL_FALSE) {
            glGetShaderInfoLog(id, 1024, nullptr, info);
            throw std::runtime_error(info);
        }

        ;
    }
}
} // namespace renderer
