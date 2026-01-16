#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <boost/call_traits.hpp>

#include <exception>
#include <iostream>
#include <memory>

#include "renderer/camera/camera.hpp"
#include "renderer/mesh/mesh.hpp"
#include "renderer/render/instanced_block_renderer.hpp"
#include "renderer/shader/shader.hpp"
#include "renderer/texture/texture.hpp"

#include "game/blocks/blocks.hpp"
#include "game/blocks/blocks_mesh_builder.hpp"

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

            // camera
            LOG_INFO("Creating camera");
            renderer::Camera camera(glm::vec3(0.0f, 0.0f, 10.0f));
            glfwSetInputMode(window.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

            glfwSetCursorPosCallback(window.get(),
            [](GLFWwindow* win, double xpos, double ypos) {
                // 需要通过user pointer传递camera
                auto* cam = static_cast<renderer::Camera*>(glfwGetWindowUserPointer(win));

                static float lastX     = 640.0f;
                static float lastY     = 360.0f;
                static bool firstMouse = true;

                if (firstMouse) {
                    lastX      = xpos;
                    lastY      = ypos;
                    firstMouse = false;
                }

                float xoffset = xpos - lastX;
                float yoffset = lastY - ypos; // 反转：y坐标从下到上

                lastX = xpos;
                lastY = ypos;

                if (cam) {
                    cam->processMouseMovement(xoffset, yoffset);
                }
            });

            glfwSetScrollCallback(window.get(),
            [](GLFWwindow* win, __attribute_maybe_unused__ double xoffset, __attribute_maybe_unused__ double yoffset) {
                auto* cam = static_cast<renderer::Camera*>(glfwGetWindowUserPointer(win));
                if (cam) {
                    cam->processMouseScroll(static_cast<float>(yoffset));
                }
            });

            glfwSetWindowUserPointer(window.get(), &camera);

            // shader
            LOG_INFO("Compiling shaders");

            renderer::shader lighting_shader(
            "resources/shaders/default/default.vert",
            "resources/shaders/default/default.frag");
            LOG_DEBUG("Shader created with ID: ", lighting_shader.get_id());


            renderer::shader instanced_shader(
            "resources/shaders/instanced/instanced.vert",
            "resources/shaders/instanced/instanced.frag");
            LOG_DEBUG("Shader created with ID: ", instanced_shader.get_id());


            // texture
            LOG_INFO("Load texture atlas");
            renderer::texture universe_atlas_texture("resources/textures/blocks/universe_block_atlas.png");
            LOG_DEBUG("Texture loaded - ID: ", universe_atlas_texture.get_id(), ", Size: ", universe_atlas_texture.get_width(), "x", universe_atlas_texture.get_height());

            // block types
            LOG_INFO("Initializing block types");
            game::blocks::initializeBlockTypes();

            // atlas metadata
            LOG_INFO("Loading atlas metadata from JSON...");
            renderer::TextureAtlas atlas;
            atlas.loadFromJSON("resources/textures/blocks/universe_block_atlas.json");

            if constexpr (false) {

                LOG_DEBUG("Loaded textures:");
                if (atlas.hasTexture("grass_top")) {
                    auto uv = atlas.getUV("grass_top");
                    LOG_DEBUG("  grass_top: (", uv.min.x, ",", uv.min.y, ") -> (", uv.max.x, ",", uv.max.y, ")");
                }
                if (atlas.hasTexture("dirt")) {
                }
                auto uv = atlas.getUV("dirt");
                LOG_DEBUG("  dirt: (", uv.min.x, ",", uv.min.y, ") -> (", uv.max.x, ",", uv.max.y, ")");
            }


            // mesh builder
            game::blocks::BlockMeshBuilder meshBuilder(&atlas);

            auto* grass_block_t = game::blocks::BlockTypeRegistry::getInstance().getBlockType("grass");
            auto* dirt_t        = game::blocks::BlockTypeRegistry::getInstance().getBlockType("dirt");
            if (!grass_block_t || !dirt_t) {
                throw std::runtime_error("Grass block type not found!");
            }

            // mesh
            LOG_INFO("Generating block meshes");
            auto grass_block_mesh = meshBuilder.generateBlockMesh(*grass_block_t);
            auto stone_mesh       = meshBuilder.generateBlockMesh(*dirt_t, glm::vec3(2.0f, 0.0f, 0.0f));

            renderer::CubeMesh::MeshData static_mesh;
            static_mesh.append(grass_block_mesh);
            static_mesh.append(stone_mesh);

            LOG_DEBUG("Mesh created - Vertices: ", static_mesh.vertices.size(), ", Indices: ", static_mesh.indices.size());

            // instanced renderer
            renderer::InstancedBlockRenderer grassRenderer(grass_block_mesh, 10000);
            std::vector<renderer::BlockInstance> instances;

            for (int x = -5; x < 5; x++) {
                for (int y = -5; y < 5; y++) {
                    for (int z = -5; z < 5; z++) {
                        instances.emplace_back(glm::vec3(x * 1.0f, y * 1.0f, z * 1.0f));
                    }
                }
            }

            LOG_INFO("Adding ", instances.size(), " block instances");
            grassRenderer.addInstances(instances);
            grassRenderer.updateInstanceBuffer();

            LOG_INFO("Instance count: ", grassRenderer.getInstanceCount());

            uint32_t VBO, VAO, EBO;
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);

            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER,
            static_mesh.vertices.size() * sizeof(renderer::Vertex),
            static_mesh.vertices.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            static_mesh.indices.size() * sizeof(uint32_t),
            static_mesh.indices.data(), GL_STATIC_DRAW);

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

            float frame_delta_time = 0.0f;
            float last_frame_time  = 0.0f;

            __attribute_maybe_unused__ size_t frame_cnt = 0;
            while (!glfwWindowShouldClose(window.get())) {

                float current_frame_time = glfwGetTime();
                frame_delta_time         = current_frame_time - last_frame_time;
                last_frame_time          = current_frame_time;
                frame_cnt++;

                if (glfwGetKey(window.get(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
                    glfwSetWindowShouldClose(window.get(), true);
                if (glfwGetKey(window.get(), GLFW_KEY_W) == GLFW_PRESS)
                    camera.processKeyboard(renderer::CameraMovement::FORWARD, frame_delta_time);
                if (glfwGetKey(window.get(), GLFW_KEY_S) == GLFW_PRESS)
                    camera.processKeyboard(renderer::CameraMovement::BACKWARD, frame_delta_time);
                if (glfwGetKey(window.get(), GLFW_KEY_A) == GLFW_PRESS)
                    camera.processKeyboard(renderer::CameraMovement::LEFT, frame_delta_time);
                if (glfwGetKey(window.get(), GLFW_KEY_D) == GLFW_PRESS)
                    camera.processKeyboard(renderer::CameraMovement::RIGHT, frame_delta_time);
                if (glfwGetKey(window.get(), GLFW_KEY_SPACE) == GLFW_PRESS)
                    camera.processKeyboard(renderer::CameraMovement::UP, frame_delta_time);
                if (glfwGetKey(window.get(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
                    camera.processKeyboard(renderer::CameraMovement::DOWN, frame_delta_time);

                GL_CHECK(glClearColor(0.2f, 0.3f, 0.3f, 1.0f));
                GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

                universe_atlas_texture.bind(0);

                glm::mat4 view       = camera.getViewMatrix();
                glm::mat4 projection = camera.getProjectionMatrix(1280.0f / 720.0f);

                instanced_shader.activate();
                instanced_shader.set("texture1", 0);
                instanced_shader.set("view", view);
                instanced_shader.set("projection", projection);
                instanced_shader.set("viewPos", camera.position);
                instanced_shader.set("lightPos", glm::vec3(1.2f, 1.0f, 2.0f));
                instanced_shader.set("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));

                grassRenderer.render();

                lighting_shader.activate();
                lighting_shader.set("texture1", 0);
                lighting_shader.set("view", view);
                lighting_shader.set("projection", projection);
                lighting_shader.set("lightPos", glm::vec3(1.2f, 1.0f, 2.0f));
                lighting_shader.set("viewPos", glm::vec3(0.0f, 0.0f, 5.0f));
                lighting_shader.set("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));


                // 绘制立方体
                GL_CHECK(glBindVertexArray(VAO));
                GL_CHECK(glDrawElements(GL_TRIANGLES, static_mesh.indices.size(), GL_UNSIGNED_INT, 0));

                glfwSwapBuffers(window.get());
                glfwPollEvents();
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
