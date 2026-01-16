#pragma once

#include "game/blocks/blocks.hpp"
#include "game/blocks/blocks_types.hpp"
#include "game/generator/perlin_noise.hpp"


#include <glm/glm.hpp>
#include <vector>

namespace game::generator {

struct TerrainBlock {
    glm::ivec3 position;
    uint32_t blockTypeId;

    TerrainBlock(const glm::ivec3& pos, uint32_t id)
    : position(pos), blockTypeId(id) {}
};

class TerrainGenerator {
    private:
    game::generator::PerlinNoise noise;

    // 地形参数
    float scale;       // 噪声缩放
    int octaves;       // 噪声层数
    float persistence; // 持久度
    int baseHeight;    // 基础高度
    int maxHeight;     // 最大高度
    int waterLevel;    // 水面高度

    public:
    TerrainGenerator(unsigned int seed = 12345)
    : noise(seed),
      scale(0.05f),
      octaves(4),
      persistence(0.5f),
      baseHeight(32),
      maxHeight(32),
      waterLevel(28) {}

    // 设置地形参数
    void setScale(float s) { scale = s; }
    void setOctaves(int o) { octaves = o; }
    void setPersistence(float p) { persistence = p; }
    void setBaseHeight(int h) { baseHeight = h; }
    void setMaxHeight(int h) { maxHeight = h; }
    void setWaterLevel(int w) { waterLevel = w; }

    // 获取指定位置的地形高度
    int getTerrainHeight(int x, int z) {
        // 使用分形布朗运动生成高度
        double noiseValue = noise.fbm(x * scale, z * scale, octaves, persistence);

        // 将噪声值(-1到1)映射到高度范围
        int height = baseHeight + (int)(noiseValue * maxHeight);

        return height;
    }

    // 生成一个区块的地形
    std::vector<TerrainBlock> generateChunk(int chunkX, int chunkZ, int chunkSize = 16) {
        std::vector<TerrainBlock> blocks;

        int startX = chunkX * chunkSize;
        int startZ = chunkZ * chunkSize;

        for (int x = 0; x < chunkSize; x++) {
            for (int z = 0; z < chunkSize; z++) {
                int worldX = startX + x;
                int worldZ = startZ + z;

                int height = getTerrainHeight(worldX, worldZ);

                // 生成从底部到地形高度的方块
                for (int y = 0; y <= height; y++) {
                    uint32_t blockType = getBlockTypeAtPosition(worldX, y, worldZ, height);

                    if (blockType != 0) { // 0是空气
                        blocks.emplace_back(glm::ivec3(worldX, y, worldZ), blockType);
                    }
                }

                // 如果地形低于水面，填充水
                if (height < waterLevel) {
                    for (int y = height + 1; y <= waterLevel; y++) {
                        blocks.emplace_back(glm::ivec3(worldX, y, worldZ),
                        ::game::blocks::BlockIDs::WATER);
                    }
                }
            }
        }

        return blocks;
    }

    // 生成平坦区域的地形
    std::vector<TerrainBlock> generateFlatTerrain(int sizeX, int sizeZ, int centerX = 0, int centerZ = 0) {
        std::vector<TerrainBlock> blocks;

        int startX = centerX - sizeX / 2;
        int startZ = centerZ - sizeZ / 2;

        for (int x = 0; x < sizeX; x++) {
            for (int z = 0; z < sizeZ; z++) {
                int worldX = startX + x;
                int worldZ = startZ + z;

                int height = getTerrainHeight(worldX, worldZ);

                // 生成地形
                for (int y = 0; y <= height; y++) {
                    uint32_t blockType = getBlockTypeAtPosition(worldX, y, worldZ, height);

                    if (blockType != 0) {
                        blocks.emplace_back(glm::ivec3(worldX, y, worldZ), blockType);
                    }
                }
            }
        }

        return blocks;
    }

    private:
    // 根据位置和高度决定方块类型
    uint32_t getBlockTypeAtPosition(int x, int y, int z, int surfaceHeight) {
        // 最底层是基岩（这里用石头代替）
        using namespace ::game::blocks::BlockIDs;
        if (y == 0) {
            return STONE;
        }

        // 表面层
        if (y == surfaceHeight) {
            // 高海拔是石头
            if (y > baseHeight + maxHeight * 0.7) {
                return STONE;
            }
            // 沙滩（靠近水面）
            else if (y <= waterLevel + 2) {
                return SAND;
            }
            // 普通草地
            else {
                return GRASS;
            }
        }

        // 地下第一层（表面下1-3格）
        if (y > surfaceHeight - 3 && y < surfaceHeight) {
            // 沙滩下是沙子
            if (surfaceHeight <= waterLevel + 2) {
                return SAND;
            }
            // 草地下是泥土
            else {
                return DIRT;
            }
        }

        // 更深的地下是石头
        if (y < surfaceHeight - 3) {
            return STONE;
        }

        return AIR; // 空气
    }
};

} // namespace game::generator