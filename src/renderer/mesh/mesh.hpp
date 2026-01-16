#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "renderer/texture/texture_atlas.hpp"

namespace renderer {

/**
 * @brief Vertex structure for mesh data
 * Contains position, normal, and texture coordinates
 */
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec4 textureBounds;
};

/**
 * @brief Utility class for generating cube/block meshes
 * Provides methods to create textured cubes with individual face textures
 */
class CubeMesh {
    public:
    /**
     * @brief Container for mesh geometry data
     * Stores vertices and indices that can be uploaded to GPU
     */
    struct MeshData {
        std::vector<Vertex> vertices;  // Vertex data array
        std::vector<uint32_t> indices; // Index array for indexed drawing

        /**
         * @brief Append another mesh to this one
         * Properly offsets indices when merging meshes
         * @param other Mesh data to append
         */
        void append(const MeshData& other);
    };

    /**
     * @brief Create a textured block with individual face textures
     *
     * Generates a unit cube centered at origin with customizable
     * textures for each face. Texture coordinates are obtained from
     * the texture atlas.
     *
     * @param atlas Texture atlas containing all textures
     * @param front_tex Texture name for front face (+Z)
     * @param back_tex Texture name for back face (-Z)
     * @param left_tex Texture name for left face (-X)
     * @param right_tex Texture name for right face (+X)
     * @param top_tex Texture name for top face (+Y)
     * @param bottom_tex Texture name for bottom face (-Y)
     * @return Generated mesh data
     */
    static MeshData createBlock(
    const TextureAtlas& atlas,
    const std::string& front_tex,
    const std::string& back_tex,
    const std::string& left_tex,
    const std::string& right_tex,
    const std::string& top_tex,
    const std::string& bottom_tex);

    private:
    /**
     * @brief Add a quad (two triangles) to the mesh
     *
     * Creates a quad from four vertices with specified texture coordinates
     * and normal. Generates two triangles in counter-clockwise winding order.
     *
     * @param mesh Mesh data to append to
     * @param uv Texture coordinates for this quad
     * @param v0 First vertex position
     * @param v1 Second vertex position
     * @param v2 Third vertex position
     * @param v3 Fourth vertex position
     * @param normal Surface normal for all vertices
     */
    static void addQuad(
    MeshData& mesh,
    const TextureUV& uv,
    const glm::vec3& v0,
    const glm::vec3& v1,
    const glm::vec3& v2,
    const glm::vec3& v3,
    const glm::vec3& normal);
};

} // namespace renderer
