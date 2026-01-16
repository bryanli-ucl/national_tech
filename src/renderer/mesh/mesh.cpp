#include "mesh.hpp"

namespace renderer {

/**
 * @brief Append another mesh to this mesh
 *
 * Combines two meshes by appending vertices and indices.
 * Indices from the appended mesh are offset to account for
 * existing vertices in this mesh.
 *
 * @param other Mesh to append
 */
void CubeMesh::MeshData::append(const MeshData& other) {
    uint32_t indexOffset = vertices.size();

    // Append vertices directly
    vertices.insert(vertices.end(), other.vertices.begin(), other.vertices.end());

    // Append indices with offset to reference correct vertices
    for (uint32_t index : other.indices) {
        indices.push_back(index + indexOffset);
    }
}

/**
 * @brief Create a textured block mesh
 *
 * Generates a unit cube (1x1x1) centered at origin with six faces.
 * Each face can have a different texture from the texture atlas.
 *
 * Cube vertex layout:
 *     7------6
 *    /|     /|
 *   3------2 |
 *   | 4----|-5
 *   |/     |/
 *   0------1
 *
 * Face normals point outward from the cube.
 *
 * @param atlas Texture atlas for UV coordinate lookup
 * @param front_tex Front face texture (+Z direction)
 * @param back_tex Back face texture (-Z direction)
 * @param left_tex Left face texture (-X direction)
 * @param right_tex Right face texture (+X direction)
 * @param top_tex Top face texture (+Y direction)
 * @param bottom_tex Bottom face texture (-Y direction)
 * @return Complete mesh data with vertices and indices
 */
CubeMesh::MeshData CubeMesh::createBlock(
const TextureAtlas& atlas,
const std::string& front_tex,
const std::string& back_tex,
const std::string& left_tex,
const std::string& right_tex,
const std::string& top_tex,
const std::string& bottom_tex) {

    MeshData mesh;

    // Define the 8 corner positions of a unit cube
    std::vector<glm::vec3> positions = {
        { -0.5f, -0.5f, -0.5f }, // 0: left-bottom-back
        { 0.5f, -0.5f, -0.5f },  // 1: right-bottom-back
        { 0.5f, 0.5f, -0.5f },   // 2: right-top-back
        { -0.5f, 0.5f, -0.5f },  // 3: left-top-back
        { -0.5f, -0.5f, 0.5f },  // 4: left-bottom-front
        { 0.5f, -0.5f, 0.5f },   // 5: right-bottom-front
        { 0.5f, 0.5f, 0.5f },    // 6: right-top-front
        { -0.5f, 0.5f, 0.5f }    // 7: left-top-front
    };

    // Reserve space for 24 vertices (4 per face) and 36 indices (6 per face)
    mesh.vertices.reserve(24);
    mesh.indices.reserve(36);

    // Add front face (+Z)
    addQuad(mesh, atlas.getUV(front_tex),
    positions[4], positions[5], positions[6], positions[7],
    { 0, 0, 1 });

    // Add back face (-Z)
    addQuad(mesh, atlas.getUV(back_tex),
    positions[1], positions[0], positions[3], positions[2],
    { 0, 0, -1 });

    // Add left face (-X)
    addQuad(mesh, atlas.getUV(left_tex),
    positions[0], positions[4], positions[7], positions[3],
    { -1, 0, 0 });

    // Add right face (+X)
    addQuad(mesh, atlas.getUV(right_tex),
    positions[5], positions[1], positions[2], positions[6],
    { 1, 0, 0 });

    // Add top face (+Y)
    addQuad(mesh, atlas.getUV(top_tex),
    positions[7], positions[6], positions[2], positions[3],
    { 0, 1, 0 });

    // Add bottom face (-Y)
    addQuad(mesh, atlas.getUV(bottom_tex),
    positions[0], positions[1], positions[5], positions[4],
    { 0, -1, 0 });

    return mesh;
}

/**
 * @brief Add a quad (four vertices forming two triangles) to mesh
 *
 * Creates a quad with proper texture coordinates and normal.
 * Vertices are wound counter-clockwise when viewed from outside.
 * Two triangles are generated: (v0, v1, v2) and (v2, v3, v0).
 *
 * @param mesh Mesh data to append quad to
 * @param uv Texture UV coordinates for the quad
 * @param v0 Bottom-left vertex position
 * @param v1 Bottom-right vertex position
 * @param v2 Top-right vertex position
 * @param v3 Top-left vertex position
 * @param normal Face normal (same for all vertices)
 */
void CubeMesh::addQuad(
MeshData& mesh,
const TextureUV& uv,
const glm::vec3& v0,
const glm::vec3& v1,
const glm::vec3& v2,
const glm::vec3& v3,
const glm::vec3& normal) {

    // 打包纹理边界
    glm::vec4 bounds(uv.min.x, uv.min.y, uv.max.x, uv.max.y);

    uint32_t base = mesh.vertices.size();

    // 添加4个顶点
    Vertex vert;
    vert.normal        = normal;
    vert.textureBounds = bounds; // 确保设置！

    vert.position = v0;
    vert.texCoord = glm::vec2(uv.min.x, uv.min.y);
    mesh.vertices.push_back(vert);

    vert.position = v1;
    vert.texCoord = glm::vec2(uv.max.x, uv.min.y);
    mesh.vertices.push_back(vert);

    vert.position = v2;
    vert.texCoord = glm::vec2(uv.max.x, uv.max.y);
    mesh.vertices.push_back(vert);

    vert.position = v3;
    vert.texCoord = glm::vec2(uv.min.x, uv.max.y);
    mesh.vertices.push_back(vert);

    // 添加索引
    mesh.indices.push_back(base + 0);
    mesh.indices.push_back(base + 1);
    mesh.indices.push_back(base + 2);

    mesh.indices.push_back(base + 0);
    mesh.indices.push_back(base + 2);
    mesh.indices.push_back(base + 3);
}

} // namespace renderer
