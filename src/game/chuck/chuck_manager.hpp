#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <unordered_map>

#include "game/chuck/chunk_mesh_optimizer.hpp"
#include "game/generator/terrain_generator.hpp"
#include "renderer/mesh/frustum.hpp"
#include "renderer/render/instanced_block_renderer.hpp"

namespace game::chuck {

// 区块坐标哈希
struct ChunkCoordHash {
    std::size_t operator()(const glm::ivec2& coord) const {
        return std::hash<int>()(coord.x) ^ (std::hash<int>()(coord.y) << 1);
    }
};

// 单个区块
struct Chunk {
    glm::ivec2 coord; // 区块坐标
    VoxelChunk voxels;
    std::unordered_map<uint32_t, renderer::CubeMesh::MeshData> meshes;
    std::unordered_map<uint32_t, std::unique_ptr<renderer::InstancedBlockRenderer>> renderers;
    renderer::AABB boundingBox;
    bool isDirty = true; // 是否需要重新生成网格

    Chunk(const glm::ivec2& c) : coord(c) {
        // 计算包围盒
        glm::vec3 worldPos(coord.x * 16.0f, 0.0f, coord.y * 16.0f);
        boundingBox.min = worldPos;
        boundingBox.max = worldPos + glm::vec3(16.0f, 256.0f, 16.0f);
    }
};

class ChunkManager {
    private:
    std::unordered_map<glm::ivec2, std::unique_ptr<Chunk>, ChunkCoordHash> chunks;
    OptimizedChunkMeshBuilder* meshBuilder;
    generator::TerrainGenerator* terrainGen;
    int renderDistance = 8; // 渲染距离（区块数）

    public:
    ChunkManager(OptimizedChunkMeshBuilder* builder, generator::TerrainGenerator* generator)
    : meshBuilder(builder), terrainGen(generator) {}

    // 设置渲染距离
    void setRenderDistance(int distance) {
        renderDistance = distance;
    }

    // 更新区块（根据玩家位置）
    void update(const glm::vec3& playerPos) {
        glm::ivec2 playerChunk(
        static_cast<int>(floor(playerPos.x / 16.0f)),
        static_cast<int>(floor(playerPos.z / 16.0f)));

        // 生成玩家周围的区块
        for (int x = -renderDistance; x <= renderDistance; x++) {
            for (int z = -renderDistance; z <= renderDistance; z++) {
                glm::ivec2 chunkCoord = playerChunk + glm::ivec2(x, z);

                // 圆形渲染距离检查
                if (x * x + z * z > renderDistance * renderDistance) {
                    continue;
                }

                if (chunks.find(chunkCoord) == chunks.end()) {
                    generateChunk(chunkCoord);
                }
            }
        }

        // TODO: 卸载远离的区块
    }

    // 渲染可见区块（使用视锥剔除）
    void render(const renderer::Frustum& frustum) {
        int visibleChunks = 0;
        int totalChunks   = chunks.size();

        for (auto& [coord, chunk] : chunks) {
            // 视锥剔除
            if (!frustum.isBoxVisible(chunk->boundingBox)) {
                continue;
            }

            visibleChunks++;

            // 如果区块是脏的，重新生成网格
            if (chunk->isDirty) {
                rebuildChunkMesh(chunk.get());
            }

            // 渲染区块中的所有方块类型
            for (auto& [typeId, renderer] : chunk->renderers) {
                if (renderer && renderer->getInstanceCount() > 0) {
                    renderer->render();
                }
            }
        }

        // 可选：输出调试信息
        // LOG_DEBUG("Chunks: ", visibleChunks, "/", totalChunks);
    }

    int getLoadedChunkCount() const {
        return chunks.size();
    }

    private:
    void generateChunk(const glm::ivec2& coord) {
        auto chunk = std::make_unique<Chunk>(coord);

        // 使用地形生成器填充区块
        auto terrainBlocks = terrainGen->generateChunk(coord.x, coord.y, 16);

        for (const auto& block : terrainBlocks) {
            int localX = block.position.x - coord.x * 16;
            int localY = block.position.y;
            int localZ = block.position.z - coord.y * 16;

            if (localX >= 0 && localX < 16 && localZ >= 0 && localZ < 16 &&
            localY >= 0 && localY < 16) {
                chunk->voxels.setBlock(localX, localY, localZ, block.blockTypeId);
            }
        }

        chunks[coord] = std::move(chunk);
    }

    void rebuildChunkMesh(Chunk* chunk) {
        // 生成优化的网格（只包含可见面）
        chunk->meshes = meshBuilder->generateChunkMesh(chunk->voxels);

        // 为每种方块类型创建渲染器
        chunk->renderers.clear();

        for (auto& [typeId, meshData] : chunk->meshes) {
            if (meshData.vertices.empty()) continue;

            // 将整个区块的网格作为一个"实例"
            // 这里不使用真正的实例化，而是直接渲染合并后的网格
            auto renderer = std::make_unique<renderer::InstancedBlockRenderer>(meshData, 1);

            // 添加一个单位变换的实例
            renderer::BlockInstance instance(glm::vec3(chunk->coord.x * 16.0f, 0.0f, chunk->coord.y * 16.0f));
            renderer->addInstance(instance);
            renderer->updateInstanceBuffer();

            chunk->renderers[typeId] = std::move(renderer);
        }

        chunk->isDirty = false;
    }
};

} // namespace game::chuck