#pragma once

#include <GLFW/glfw3.h>

#include "utils/logger.hpp"

void checkGLError(const char* stmt, const char* fname, int line) {
    GLenum err = glGetError();
    while (err != GL_NO_ERROR) {
        LOG_ERROR("OpenGL error ", err, " at ", fname, ":", line, " - for ", stmt);
        err = glGetError();
    }
}

#define GL_CHECK(stmt)                           \
    do {                                         \
        stmt;                                    \
        checkGLError(#stmt, __FILE__, __LINE__); \
    } while (0)
