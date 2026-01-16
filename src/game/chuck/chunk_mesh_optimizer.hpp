#pragma once

#include <glm/glm.hpp>

#include <unordered_map>
#include <vector>

#include "game/blocks/blocks_types.hpp"
#include "renderer/mesh/mesh.hpp"

namespace game::chuck {

// 简化的方块世界表示
class VoxelChunk {
    private:
    static constexpr int CHUNK_SIZE_X = 16;
    static constexpr int CHUNK_SIZE_Y = 256;
    static constexpr int CHUNK_SIZE_Z = 16;

    std::vector<uint32_t> blocks;
    int getIndex(int x, int y, int z) const {
        return x + y * CHUNK_SIZE_X + z * CHUNK_SIZE_X * CHUNK_SIZE_Y;
    }

    public:
    VoxelChunk() {
        blocks.resize(CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z, 0);
    }

    void setBlock(int x, int y, int z, uint32_t typeId) {
        if (x >= 0 && x < CHUNK_SIZE_X &&
        y >= 0 && y < CHUNK_SIZE_Y &&
        z >= 0 && z < CHUNK_SIZE_Z) {
            blocks[getIndex(x, y, z)] = typeId;
        }
    }

    uint32_t getBlock(int x, int y, int z) const {
        if (x < 0 || x >= CHUNK_SIZE_X ||
        y < 0 || y >= CHUNK_SIZE_Y ||
        z < 0 || z >= CHUNK_SIZE_Z) {
            return 0;
        }
        return blocks[getIndex(x, y, z)];
    }

    bool isBlockSolid(int x, int y, int z) const {
        uint32_t typeId = getBlock(x, y, z);
        if (typeId == 0) return false;

        auto* blockType = blocks::BlockTypeRegistry::getInstance().getBlockType(typeId);
        return blockType ? blockType->isSolid : false;
    }

    // 检查某个面是否需要渲染（相邻方块是否遮挡）
    bool shouldRenderFace(int x, int y, int z, blocks::BlockFace face) const {
        if (getBlock(x, y, z) == 0) return false; // 空气不渲染

        using namespace blocks;

        // 检查相邻方块
        int nx = x, ny = y, nz = z;
        switch (face) {
        case BlockFace::FRONT: nz++; break;
        case BlockFace::BACK: nz--; break;
        case BlockFace::LEFT: nx--; break;
        case BlockFace::RIGHT: nx++; break;
        case BlockFace::TOP: ny++; break;
        case BlockFace::BOTTOM: ny--; break;
        }

        // 如果相邻是空气或透明方块，需要渲染这个面
        return !isBlockSolid(nx, ny, nz);
    }

    int getSizeX() const { return CHUNK_SIZE_X; }
    int getSizeY() const { return CHUNK_SIZE_Y; }
    int getSizeZ() const { return CHUNK_SIZE_Z; }
};

// 优化的区块网格生成器
class OptimizedChunkMeshBuilder {
    private:
    renderer::TextureAtlas* atlas;

    public:
    OptimizedChunkMeshBuilder(renderer::TextureAtlas* textureAtlas)
    : atlas(textureAtlas) {}

    std::unordered_map<uint32_t, renderer::CubeMesh::MeshData> generateChunkMesh(const VoxelChunk& chunk) {
        std::unordered_map<uint32_t, renderer::CubeMesh::MeshData> meshes;

        int sizeX = chunk.getSizeX();
        int sizeY = chunk.getSizeY();
        int sizeZ = chunk.getSizeZ();

        for (int x = 0; x < sizeX; x++) {
            for (int y = 0; y < sizeY; y++) {
                for (int z = 0; z < sizeZ; z++) {
                    uint32_t typeId = chunk.getBlock(x, y, z);
                    if (typeId == 0) continue;

                    auto* blockType = blocks::BlockTypeRegistry::getInstance().getBlockType(typeId);
                    if (!blockType) continue;

                    for (int faceIdx = 0; faceIdx < 6; faceIdx++) {
                        blocks::BlockFace face = static_cast<blocks::BlockFace>(faceIdx);

                        if (chunk.shouldRenderFace(x, y, z, face)) {
                            addBlockFace(meshes[typeId], *blockType,
                            glm::vec3(x, y, z), face);
                        }
                    }
                }
            }
        }

        return meshes;
    }

    private:
    void addBlockFace(renderer::CubeMesh::MeshData& meshData, const blocks::BlockType& blockType, const glm::vec3& position, blocks::BlockFace face) {

        // 立方体顶点（相对位置）
        glm::vec3 vertices[8] = {
            position + glm::vec3(-0.5f, -0.5f, -0.5f),
            position + glm::vec3(0.5f, -0.5f, -0.5f),
            position + glm::vec3(0.5f, 0.5f, -0.5f),
            position + glm::vec3(-0.5f, 0.5f, -0.5f),
            position + glm::vec3(-0.5f, -0.5f, 0.5f),
            position + glm::vec3(0.5f, -0.5f, 0.5f),
            position + glm::vec3(0.5f, 0.5f, 0.5f),
            position + glm::vec3(-0.5f, 0.5f, 0.5f)
        };

        // 获取纹理UV
        const std::string& textureName = blockType.getTexture(face);
        renderer::TextureUV uv         = atlas->getUV(textureName);

        glm::vec2 uvCoords[4] = {
            glm::vec2(uv.min.x, uv.min.y),
            glm::vec2(uv.max.x, uv.min.y),
            glm::vec2(uv.max.x, uv.max.y),
            glm::vec2(uv.min.x, uv.max.y)
        };

        // 根据面选择顶点和法线
        std::array<int, 4> indices;
        glm::vec3 normal;

        using namespace blocks;

        switch (face) {
        case BlockFace::FRONT:
            indices = { 4, 5, 6, 7 };
            normal  = glm::vec3(0, 0, 1);
            break;
        case BlockFace::BACK:
            indices = { 1, 0, 3, 2 };
            normal  = glm::vec3(0, 0, -1);
            break;
        case BlockFace::LEFT:
            indices = { 0, 4, 7, 3 };
            normal  = glm::vec3(-1, 0, 0);
            break;
        case BlockFace::RIGHT:
            indices = { 5, 1, 2, 6 };
            normal  = glm::vec3(1, 0, 0);
            break;
        case BlockFace::TOP:
            indices = { 7, 6, 2, 3 };
            normal  = glm::vec3(0, 1, 0);
            break;
        case BlockFace::BOTTOM:
            indices = { 0, 1, 5, 4 };
            normal  = glm::vec3(0, -1, 0);
            break;
        }

        uint32_t baseIndex = meshData.vertices.size();

        // 添加4个顶点
        for (int i = 0; i < 4; i++) {
            renderer::Vertex vertex;
            vertex.position = vertices[indices[i]];
            vertex.normal   = normal;
            vertex.texCoord = uvCoords[i];
            meshData.vertices.push_back(vertex);
        }

        // 添加2个三角形
        meshData.indices.push_back(baseIndex + 0);
        meshData.indices.push_back(baseIndex + 1);
        meshData.indices.push_back(baseIndex + 2);

        meshData.indices.push_back(baseIndex + 0);
        meshData.indices.push_back(baseIndex + 2);
        meshData.indices.push_back(baseIndex + 3);
    }
};

} // namespace game::chuck