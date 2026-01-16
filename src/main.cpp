#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <boost/call_traits.hpp>

#include <exception>
#include <iostream>
#include <memory>

#include "renderer/mesh/indexed_mesh.hpp"
#include "renderer/shader/shader.hpp"
#include "renderer/texture/texture.hpp"

// 错误检查函数
void checkGLError(const char* stmt, const char* fname, int line) {
    GLenum err = glGetError();
    while (err != GL_NO_ERROR) {
        std::cerr << "OpenGL error " << err << " at " << fname << ":" << line
                  << " - for " << stmt << std::endl;
        err = glGetError();
    }
}

#define GL_CHECK(stmt)                           \
    do {                                         \
        stmt;                                    \
        checkGLError(#stmt, __FILE__, __LINE__); \
    } while (0)

auto main(__attribute_maybe_unused__ int argc, __attribute_maybe_unused__ char** argv) -> int {

    try {
        glfwInit();

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        std::shared_ptr<GLFWwindow> window(
        glfwCreateWindow(1280, 720, "National Technology", nullptr, nullptr),
        [](GLFWwindow* win) {
            glfwDestroyWindow(win);
        });
        if (window == nullptr)
            throw std::runtime_error("cannot create GL window");

        glfwMakeContextCurrent(window.get());
        glfwSetFramebufferSizeCallback(window.get(), [](__attribute_maybe_unused__ GLFWwindow* win, int width, int height) {
            glViewport(0, 0, width, height);
        });

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
            throw std::runtime_error("cannot initialize GLAD");

        GLint maxTextureSize;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
        std::cout << "Max texture size: " << maxTextureSize << "x" << maxTextureSize << std::endl;

        glEnable(GL_DEPTH_TEST);

        {
            // shader
            std::cout << "Creating shader..." << std::endl;

            renderer::shader lighting_shader(
            "resources/shaders/default/default.vert",
            "resources/shaders/default/default.frag");

            std::cout << "Shader created with ID: " << lighting_shader.get_id() << std::endl;

            // texture
            std::cout << "Loading texture atlas..." << std::endl;
            renderer::texture atlas_texture("resources/textures/blocks/block_atlas.png");
            std::cout << "Texture loaded - ID: " << atlas_texture.get_id()
                      << ", Size: " << atlas_texture.get_width() << "x" << atlas_texture.get_height() << std::endl;

            // atlas metadata
            std::cout << "Loading atlas metadata..." << std::endl;
            renderer::TextureAtlas atlas;
            atlas.setupFromImageSize(
            atlas_texture.get_width(),
            atlas_texture.get_height(),
            16 // 纹理大小
            );

            std::cout << "\nRegistering textures:" << std::endl;
            atlas.registerTexture("grass_top", 8);  // 第一个纹理
            atlas.registerTexture("grass_side", 5); // 第二个纹理
            atlas.registerTexture("dirt", 6);       // 第三个纹理

            /*
            6 7 8
            3 4 5
            0 1 2
            */

            // mesh
            std::cout << "Generating grass block mesh..." << std::endl;
            auto meshData = renderer::IndexedCubeMesh::createGrassBlockFromAtlas(atlas);

            std::cout << "Mesh created - Vertices: " << meshData.vertices.size()
                      << ", Indices: " << meshData.indices.size() << std::endl;

            std::cout << "Sample UV coordinates:" << std::endl;
            for (size_t i = 0; i < std::min(size_t(6), meshData.vertices.size()); ++i) {
                std::cout << "  Vertex " << i << ": UV("
                          << meshData.vertices[i].texCoord.x << ", "
                          << meshData.vertices[i].texCoord.y << ")" << std::endl;
            }

            uint32_t VBO, VAO, EBO;
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);

            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER,
            meshData.vertices.size() * sizeof(renderer::Vertex),
            meshData.vertices.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            meshData.indices.size() * sizeof(uint32_t),
            meshData.indices.data(), GL_STATIC_DRAW);

            // 位置属性
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(renderer::Vertex),
            (void*)offsetof(renderer::Vertex, position));
            glEnableVertexAttribArray(0);

            // 法线属性
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(renderer::Vertex),
            (void*)offsetof(renderer::Vertex, normal));
            glEnableVertexAttribArray(1);

            // 纹理坐标属性
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(renderer::Vertex),
            (void*)offsetof(renderer::Vertex, texCoord));
            glEnableVertexAttribArray(2);

            std::cout << "VAO setup complete" << std::endl;
            std::cout << "Entering render loop..." << std::endl;

            // 光照和相机参数
            glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
            glm::vec3 viewPos(0.0f, 0.0f, 3.0f);

            std::cout << "Entering render loop..." << std::endl;

            int frameCount = 0;
            while (!glfwWindowShouldClose(window.get())) {

                if (glfwGetKey(window.get(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
                    glfwSetWindowShouldClose(window.get(), true);

                GL_CHECK(glClearColor(0.2f, 0.3f, 0.3f, 1.0f));
                GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

                lighting_shader.activate();

                atlas_texture.bind(0);
                lighting_shader.set("texture1", 0);

                lighting_shader.set("lightPos", lightPos);
                lighting_shader.set("viewPos", viewPos);
                lighting_shader.set("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
                lighting_shader.set("objectColor", glm::vec3(1.0f, 0.5f, 0.31f));

                glm::mat4 model = glm::rotate(glm::mat4(1.0f), (float)glfwGetTime(), glm::vec3(0.5f, 1.0f, 0.0f));

                glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));

                glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f);

                lighting_shader.set("model", model);
                lighting_shader.set("view", view);
                lighting_shader.set("projection", projection);

                lighting_shader.set("lightPos", glm::vec3(1.2f, 1.0f, 2.0f));
                lighting_shader.set("viewPos", glm::vec3(0.0f, 0.0f, 3.0f));
                lighting_shader.set("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));

                // 绘制立方体
                GL_CHECK(glBindVertexArray(VAO));
                GL_CHECK(glDrawElements(GL_TRIANGLES, meshData.indices.size(), GL_UNSIGNED_INT, 0));

                if (frameCount < 5 || frameCount % 100 == 0) {
                    std::cout << "Frame " << frameCount << " rendered" << std::endl;
                }

                if (frameCount == 0) {
                    std::cout << "\n=== Mesh Debug Info ===" << std::endl;
                    std::cout << "Vertices count: " << meshData.vertices.size() << std::endl;
                    std::cout << "Indices count: " << meshData.indices.size() << std::endl;

                    // 打印前几个顶点的纹理坐标
                    std::cout << "First 4 vertices UV coords:" << std::endl;
                    for (size_t i = 0; i < std::min(size_t(4), meshData.vertices.size()); ++i) {
                        auto& v = meshData.vertices[i];
                        std::cout << "  Vertex " << i << ": UV("
                                  << v.texCoord.x << ", " << v.texCoord.y << ")" << std::endl;
                    }
                    std::cout << "======================\n"
                              << std::endl;
                }

                glfwSwapBuffers(window.get());
                glfwPollEvents();
                frameCount++;
            }

            std::cout << "Exited render loop" << std::endl;
            std::cout << "Cleaning up..." << std::endl;
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            std::cout << "Cleanup complete" << std::endl;
        }

        glfwTerminate();
        std::cout << "Terminating GLFW..." << std::endl;

        std::cout << "Program ended successfully!" << std::endl;
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}
