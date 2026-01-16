#pragma once

#include <glm/glm.hpp>

#include <vector>

#include "renderer/mesh/mesh.hpp"
#include "renderer/texture/texture.hpp"

#include "blocks_types.hpp"

namespace game::blocks {

class BlockMeshBuilder {
    private:
    renderer::TextureAtlas* atlas;

    public:
    BlockMeshBuilder(renderer::TextureAtlas* textureAtlas)
    : atlas(textureAtlas) {}

    // 生成单个方块的网格数据
    renderer::CubeMesh::MeshData generateBlockMesh(const BlockType& blockType, const glm::vec3& position = glm::vec3(0.0f)) {
        renderer::CubeMesh::MeshData meshData;

        // 立方体的8个顶点（相对于position）
        glm::vec3 vertices[8] = {
            position + glm::vec3(-0.5f, -0.5f, -0.5f), // 0: 左下后
            position + glm::vec3(0.5f, -0.5f, -0.5f),  // 1: 右下后
            position + glm::vec3(0.5f, 0.5f, -0.5f),   // 2: 右上后
            position + glm::vec3(-0.5f, 0.5f, -0.5f),  // 3: 左上后
            position + glm::vec3(-0.5f, -0.5f, 0.5f),  // 4: 左下前
            position + glm::vec3(0.5f, -0.5f, 0.5f),   // 5: 右下前
            position + glm::vec3(0.5f, 0.5f, 0.5f),    // 6: 右上前
            position + glm::vec3(-0.5f, 0.5f, 0.5f)    // 7: 左上前
        };

        // 为每个面添加顶点
        addFace(meshData, blockType, BlockFace::FRONT, vertices, { 4, 5, 6, 7 }, glm::vec3(0, 0, 1));
        addFace(meshData, blockType, BlockFace::BACK, vertices, { 1, 0, 3, 2 }, glm::vec3(0, 0, -1));
        addFace(meshData, blockType, BlockFace::LEFT, vertices, { 0, 4, 7, 3 }, glm::vec3(-1, 0, 0));
        addFace(meshData, blockType, BlockFace::RIGHT, vertices, { 5, 1, 2, 6 }, glm::vec3(1, 0, 0));
        addFace(meshData, blockType, BlockFace::TOP, vertices, { 7, 6, 2, 3 }, glm::vec3(0, 1, 0));
        addFace(meshData, blockType, BlockFace::BOTTOM, vertices, { 0, 1, 5, 4 }, glm::vec3(0, -1, 0));

        return meshData;
    }

    private:
    void addFace(renderer::CubeMesh::MeshData& meshData, const BlockType& blockType, BlockFace face, glm::vec3 vertices[8], std::array<int, 4> indices, const glm::vec3& normal) {

        // 获取该面的纹理UV坐标
        const std::string& textureName = blockType.getTexture(face);
        renderer::TextureUV uv         = atlas->getUV(textureName);

        // UV坐标（逆时针）
        glm::vec2 uvCoords[4] = {
            glm::vec2(uv.min.x, uv.min.y), // 左下
            glm::vec2(uv.max.x, uv.min.y), // 右下
            glm::vec2(uv.max.x, uv.max.y), // 右上
            glm::vec2(uv.min.x, uv.max.y)  // 左上
        };

        uint32_t baseIndex = meshData.vertices.size();

        // 添加4个顶点
        for (int i = 0; i < 4; i++) {
            renderer::Vertex vertex;
            vertex.position = vertices[indices[i]];
            vertex.normal   = normal;
            vertex.texCoord = uvCoords[i];
            meshData.vertices.push_back(vertex);
        }

        // 添加2个三角形（6个索引）
        meshData.indices.push_back(baseIndex + 0);
        meshData.indices.push_back(baseIndex + 1);
        meshData.indices.push_back(baseIndex + 2);

        meshData.indices.push_back(baseIndex + 0);
        meshData.indices.push_back(baseIndex + 2);
        meshData.indices.push_back(baseIndex + 3);
    }
};

} // namespace game::blocks
