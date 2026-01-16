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

#include "utils/check.hpp"

auto main(__attribute_maybe_unused__ int argc, __attribute_maybe_unused__ char** argv) -> int {

    try {

        utils::log().setLevel(utils::LogLevel::DEBUG);

        LOG_SEPARATOR();
        LOG_INFO("NATIONAL TECHNOLOGY STARTING");

        glfwInit();

        LOG_DEBUG("Set GLFW version");
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        LOG_INFO("Creating game window");
        std::shared_ptr<GLFWwindow> window(
        glfwCreateWindow(1280, 720, "National Technology", nullptr, nullptr),
        [](GLFWwindow* win) {
            glfwDestroyWindow(win);
        });
        if (window == nullptr)
            throw std::runtime_error("cannot create GL window");

        glfwMakeContextCurrent(window.get());

        LOG_DEBUG("Set window callback function");
        glfwSetFramebufferSizeCallback(window.get(), [](__attribute_maybe_unused__ GLFWwindow* win, int width, int height) {
            glViewport(0, 0, width, height);
        });

        LOG_DEBUG("Set glad");
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
            throw std::runtime_error("cannot initialize GLAD");

        GLint maxTextureSize;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
        LOG_DEBUG("Max texture size: ", maxTextureSize, "x", maxTextureSize);

        glEnable(GL_DEPTH_TEST);

        LOG_SEPARATOR();

        {
            // shader
            LOG_INFO("Compiling shaders");

            renderer::shader lighting_shader(
            "resources/shaders/default/default.vert",
            "resources/shaders/default/default.frag");

            LOG_DEBUG("Shader created with ID: ", lighting_shader.get_id());

            // texture
            LOG_INFO("Load texture atlas");
            renderer::texture atlas_texture("resources/textures/blocks/universe_block_atlas.png");
            LOG_DEBUG("Texture loaded - ID: ", atlas_texture.get_id(), ", Size: ", atlas_texture.get_width(), "x", atlas_texture.get_height());

            // atlas metadata
            LOG_INFO("Loading atlas metadata from JSON...");
            renderer::TextureAtlas atlas;
            atlas.loadFromJSON("resources/textures/blocks/universe_block_atlas.json");

            LOG_DEBUG("Loaded textures:");
            if (atlas.hasTexture("grass_top")) {
                auto uv = atlas.getUV("grass_top");
                LOG_DEBUG("  grass_top: (", uv.min.x, ",", uv.min.y, ") -> (", uv.max.x, ",", uv.max.y, ")");
            }
            if (atlas.hasTexture("dirt")) {
                auto uv = atlas.getUV("dirt");
                LOG_DEBUG("  dirt: (", uv.min.x, ",", uv.min.y, ") -> (", uv.max.x, ",", uv.max.y, ")");
            }

            // mesh
            LOG_INFO("Generating block meshes");
            auto meshData = renderer::IndexedCubeMesh::createGrassBlockFromAtlas(atlas);
            LOG_DEBUG("Mesh created - Vertices: ", meshData.vertices.size(), ", Indices: ", meshData.indices.size());

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

            LOG_INFO("VAO, VBO, EBO setup");

            LOG_SEPARATOR();
            LOG_INFO("GAME START");

            // 光照和相机参数
            glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
            glm::vec3 viewPos(0.0f, 0.0f, 10.0f);

            __attribute_maybe_unused__ size_t frame_cnt = 0;
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

                // glm::mat4 model = glm::mat4(1.0f);
                glm::mat4 model      = glm::rotate(glm::mat4(1.0f), (float)glfwGetTime(), glm::vec3(0.5f, 1.0f, 0.0f));
                glm::mat4 view       = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -10.0f));
                glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f);

                lighting_shader.set("model", model);
                lighting_shader.set("view", view);
                lighting_shader.set("projection", projection);

                lighting_shader.set("lightPos", glm::vec3(1.2f, 1.0f, 2.0f));
                lighting_shader.set("viewPos", glm::vec3(0.0f, 0.0f, 5.0f));
                lighting_shader.set("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));

                // 绘制立方体
                GL_CHECK(glBindVertexArray(VAO));
                GL_CHECK(glDrawElements(GL_TRIANGLES, meshData.indices.size(), GL_UNSIGNED_INT, 0));

                glfwSwapBuffers(window.get());
                glfwPollEvents();

                frame_cnt++;
            }

            LOG_INFO("Exit game loop");

            LOG_INFO("Cleaning up");
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
        }

        LOG_INFO("Shut down");
        glfwTerminate();

    } catch (std::exception& e) {
        LOG_FATAL("FATAL EXCEPTION: ", e.what());
        glfwTerminate();
    }
    return 0;
}
