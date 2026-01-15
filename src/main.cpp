#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <exception>
#include <iostream>
#include <memory>


auto main(__attribute_maybe_unused__ int argc, __attribute_maybe_unused__ char** argv) -> int {

    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    std::shared_ptr<GLFWwindow> window(
    glfwCreateWindow(1280, 720, "National Technology", nullptr, nullptr),
    [](GLFWwindow* win) {
        glfwDestroyWindow(win);
    });
    if (window == nullptr)
        throw std::runtime_error("cannot create GL window");

    glfwMakeContextCurrent(window.get());
    glfwSetFramebufferSizeCallback(window.get(), [](GLFWwindow* win, int width, int height) {
        glViewport(0, 0, width, height);
    });

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        throw std::runtime_error("cannot initialize GLAD");

    glEnable(GL_DEPTH_TEST);


    while (!glfwWindowShouldClose(window.get())) {

        if (glfwGetKey(window.get(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window.get(), true);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glfwSwapBuffers(window.get());
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}
