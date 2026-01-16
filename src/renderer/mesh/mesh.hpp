#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "renderer/texture/texture_atlas.hpp"

namespace renderer {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

class CubeMesh {
    public:
    struct MeshData {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        void append(const MeshData& other) {
            uint32_t indexOffset = vertices.size();

            // 添加顶点
            vertices.insert(vertices.end(), other.vertices.begin(), other.vertices.end());

            // 添加索引（需要偏移）
            for (uint32_t index : other.indices) {
                indices.push_back(index + indexOffset);
            }
        }
    };

    // 创建任意方块（指定每个面的纹理名称）
    static MeshData createBlock(
    const TextureAtlas& atlas,
    const std::string& front_tex,
    const std::string& back_tex,
    const std::string& left_tex,
    const std::string& right_tex,
    const std::string& top_tex,
    const std::string& bottom_tex) {
        MeshData mesh;

        std::vector<glm::vec3> positions = {
            { -0.5f, -0.5f, -0.5f }, { 0.5f, -0.5f, -0.5f },
            { 0.5f, 0.5f, -0.5f }, { -0.5f, 0.5f, -0.5f },
            { -0.5f, -0.5f, 0.5f }, { 0.5f, -0.5f, 0.5f },
            { 0.5f, 0.5f, 0.5f }, { -0.5f, 0.5f, 0.5f }
        };

        mesh.vertices.reserve(24);
        mesh.indices.reserve(36);

        addQuad(mesh, atlas.getUV(front_tex), positions[4], positions[5], positions[6], positions[7], { 0, 0, 1 });
        addQuad(mesh, atlas.getUV(back_tex), positions[1], positions[0], positions[3], positions[2], { 0, 0, -1 });
        addQuad(mesh, atlas.getUV(left_tex), positions[0], positions[4], positions[7], positions[3], { -1, 0, 0 });
        addQuad(mesh, atlas.getUV(right_tex), positions[5], positions[1], positions[2], positions[6], { 1, 0, 0 });
        addQuad(mesh, atlas.getUV(top_tex), positions[7], positions[6], positions[2], positions[3], { 0, 1, 0 });
        addQuad(mesh, atlas.getUV(bottom_tex), positions[0], positions[1], positions[5], positions[4], { 0, -1, 0 });

        return mesh;
    }

    private:
    static void addQuad(
    MeshData& mesh,
    const TextureUV& uv,
    const glm::vec3& v0,
    const glm::vec3& v1,
    const glm::vec3& v2,
    const glm::vec3& v3,
    const glm::vec3& normal) {
        uint32_t startIdx = mesh.vertices.size();

        mesh.vertices.push_back({ v0, normal, glm::vec2(uv.min.x, uv.min.y) });
        mesh.vertices.push_back({ v1, normal, glm::vec2(uv.max.x, uv.min.y) });
        mesh.vertices.push_back({ v2, normal, glm::vec2(uv.max.x, uv.max.y) });
        mesh.vertices.push_back({ v3, normal, glm::vec2(uv.min.x, uv.max.y) });

        mesh.indices.insert(mesh.indices.end(), { startIdx + 0, startIdx + 1, startIdx + 2, startIdx + 2, startIdx + 3, startIdx + 0 });
    }
};

} // namespace renderer