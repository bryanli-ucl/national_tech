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

class IndexedCubeMesh {
    public:
    struct MeshData {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };

    // 使用纹理图集创建草方块
    static MeshData createGrassBlockFromAtlas(const TextureAtlas& atlas) {
        MeshData mesh;

        // 从图集获取 UV 坐标
        TextureUV topUV    = atlas.getUV("grass_top");
        TextureUV sideUV   = atlas.getUV("grass_side");
        TextureUV bottomUV = atlas.getUV("dirt");

        // 8 个顶点位置
        std::vector<glm::vec3> positions = {
            { -0.5f, -0.5f, -0.5f }, // 0
            { 0.5f, -0.5f, -0.5f },  // 1
            { 0.5f, 0.5f, -0.5f },   // 2
            { -0.5f, 0.5f, -0.5f },  // 3
            { -0.5f, -0.5f, 0.5f },  // 4
            { 0.5f, -0.5f, 0.5f },   // 5
            { 0.5f, 0.5f, 0.5f },    // 6
            { -0.5f, 0.5f, 0.5f }    // 7
        };

        mesh.vertices.reserve(24);
        mesh.indices.reserve(36);

        // 前面 (Z+)
        addQuad(mesh, sideUV, positions[4], positions[5], positions[6], positions[7], { 0, 0, 1 });

        // 后面 (Z-)
        addQuad(mesh, sideUV, positions[1], positions[0], positions[3], positions[2], { 0, 0, -1 });

        // 左面 (X-)
        addQuad(mesh, sideUV, positions[0], positions[4], positions[7], positions[3], { -1, 0, 0 });

        // 右面 (X+)
        addQuad(mesh, sideUV, positions[5], positions[1], positions[2], positions[6], { 1, 0, 0 });

        // 顶面 (Y+)
        addQuad(mesh, topUV, positions[7], positions[6], positions[2], positions[3], { 0, 1, 0 });

        // 底面 (Y-)
        addQuad(mesh, bottomUV, positions[0], positions[1], positions[5], positions[4], { 0, -1, 0 });

        return mesh;
    }

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