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
#include "renderer/mesh/frustum.hpp"
#include "renderer/mesh/mesh.hpp"
#include "renderer/render/instanced_block_renderer.hpp"
#include "renderer/shader/shader.hpp"
#include "renderer/texture/texture.hpp"

#include "game/blocks/blocks.hpp"
#include "game/blocks/blocks_mesh_builder.hpp"
#include "game/chuck/chuck_manager.hpp"
#include "game/chuck/chunk_mesh_optimizer.hpp"
#include "game/generator/terrain_generator.hpp"


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
        glfwCreateWindow(1920, 1080, "National Technology", nullptr, nullptr),
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
        glEnable(GL_CULL_FACE);

        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        LOG_SEPARATOR();

        {
            // camera
            LOG_INFO("Creating camera");
            renderer::Camera camera(glm::vec3(0.0f, 25.0f, 10.0f));
            renderer::Frustum frustum;
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

            // terrain generater
            LOG_INFO("Creating terrain generator");
            game::generator::TerrainGenerator terr_gen(1);

            terr_gen.setScale(0.03f);   // 地形的"放大"程度，越小越平缓
            terr_gen.setOctaves(1);     // 细节层次，越多越复杂
            terr_gen.setBaseHeight(50); // 基础高度
            terr_gen.setMaxHeight(80);  // 最大高度变化
            terr_gen.setWaterLevel(18); // 水面高度

            LOG_INFO("Generating terrain...");
            auto terrainBlocks = terr_gen.generateFlatTerrain(1024, 1024, 0, 0);
            LOG_INFO("Generated ", terrainBlocks.size(), " terrain blocks");

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
            game::chuck::OptimizedChunkMeshBuilder meshBuilder(&atlas);

            game::chuck::ChunkManager chunkManager(&meshBuilder, &terr_gen);
            chunkManager.setRenderDistance(8); // 8个区块的渲染距离

            LOG_SEPARATOR();
            LOG_INFO("GAME START");

            float frame_delta_time = 0.0f;
            float last_frame_time  = 0.0f;

            __attribute_maybe_unused__ size_t frame_cnt = 0;
            while (!glfwWindowShouldClose(window.get())) {

                // frame time stat
                float current_frame_time = glfwGetTime();
                frame_delta_time         = current_frame_time - last_frame_time;
                last_frame_time          = current_frame_time;
                frame_cnt++;

                // input process
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

                // 每10帧更新一次区块加载
                if (frame_cnt % 10 == 0) {
                    chunkManager.update(camera.position);
                }

                GL_CHECK(glClearColor(0.2f, 0.3f, 0.3f, 1.0f));
                GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

                universe_atlas_texture.bind(0);

                instanced_shader.activate();
                instanced_shader.set("texture1", 0);

                glm::mat4 view       = camera.getViewMatrix();
                glm::mat4 projection = camera.getProjectionMatrix(1280.0f / 720.0f);

                frustum.extractFromMatrix(projection * view);

                instanced_shader.set("view", view);
                instanced_shader.set("projection", projection);
                instanced_shader.set("viewPos", camera.position);
                instanced_shader.set("lightPos", glm::vec3(100.0f, 100.0f, 2.0f));
                instanced_shader.set("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));

                chunkManager.render(frustum);

                glfwSwapBuffers(window.get());
                glfwPollEvents();
            }

            LOG_INFO("Exit game loop");

            LOG_INFO("Cleaning up");
        }

        LOG_INFO("Shut down");
        glfwTerminate();

    } catch (std::exception& e) {
        LOG_FATAL("FATAL EXCEPTION: ", e.what());
        glfwTerminate();
    }
    return 0;
}
