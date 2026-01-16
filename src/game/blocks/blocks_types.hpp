#pragma once

#include <string>

#include "renderer/texture/texture.hpp"
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace game::blocks {

// 方块面的枚举
enum class BlockFace {
    FRONT = 0,
    BACK,
    LEFT,
    RIGHT,
    TOP,
    BOTTOM
};

// 单个方块类型的定义
struct BlockType {
    uint32_t id;
    std::string name;

    // 每个面对应的纹理名称
    std::string textures[6];

    // 方块属性
    bool isTransparent;
    bool isSolid;
    float hardness;

    BlockType(uint32_t id, const std::string& name)
    : id(id), name(name), isTransparent(false), isSolid(true), hardness(1.0f) {
        // 默认所有面使用同一纹理
        for (int i = 0; i < 6; i++) {
            textures[i] = name;
        }
    }

    // 设置所有面使用同一纹理
    BlockType& setTexture(const std::string& textureName) {
        for (int i = 0; i < 6; i++) {
            textures[i] = textureName;
        }
        return *this;
    }

    // 设置特定面的纹理
    BlockType& setFaceTexture(BlockFace face, const std::string& textureName) {
        textures[static_cast<int>(face)] = textureName;
        return *this;
    }

    // 设置顶部和侧面纹理（常用于草方块等）
    BlockType& setTopSideBottom(const std::string& top, const std::string& side, const std::string& bottom) {
        textures[static_cast<int>(BlockFace::TOP)]    = top;
        textures[static_cast<int>(BlockFace::BOTTOM)] = bottom;
        textures[static_cast<int>(BlockFace::FRONT)]  = side;
        textures[static_cast<int>(BlockFace::BACK)]   = side;
        textures[static_cast<int>(BlockFace::LEFT)]   = side;
        textures[static_cast<int>(BlockFace::RIGHT)]  = side;
        return *this;
    }

    // 获取指定面的纹理名称
    const std::string& getTexture(BlockFace face) const {
        return textures[static_cast<int>(face)];
    }

    // 设置透明度
    BlockType& setTransparent(bool transparent) {
        isTransparent = transparent;
        return *this;
    }

    // 设置是否为实心
    BlockType& setSolid(bool solid) {
        isSolid = solid;
        return *this;
    }

    // 设置硬度
    BlockType& setHardness(float h) {
        hardness = h;
        return *this;
    }
};

// 方块类型管理器
class BlockTypeRegistry {
    private:
    std::unordered_map<uint32_t, std::unique_ptr<BlockType>> blockTypesById;
    std::unordered_map<std::string, BlockType*> blockTypesByName;
    uint32_t nextId;

    BlockTypeRegistry() : nextId(1) {} // ID 0 保留给空气

    public:
    // 单例模式
    static BlockTypeRegistry& getInstance() {
        static BlockTypeRegistry instance;
        return instance;
    }

    // 注册方块类型
    BlockType& registerBlock(const std::string& name) {
        uint32_t id    = nextId++;
        auto blockType = std::make_unique<BlockType>(id, name);
        BlockType* ptr = blockType.get();

        blockTypesById[id]     = std::move(blockType);
        blockTypesByName[name] = ptr;

        return *ptr;
    }

    // 通过ID获取方块类型
    BlockType* getBlockType(uint32_t id) {
        auto it = blockTypesById.find(id);
        if (it != blockTypesById.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    // 通过名称获取方块类型
    BlockType* getBlockType(const std::string& name) {
        auto it = blockTypesByName.find(name);
        if (it != blockTypesByName.end()) {
            return it->second;
        }
        return nullptr;
    }

    // 获取所有方块类型
    const std::unordered_map<uint32_t, std::unique_ptr<BlockType>>& getAllBlockTypes() const {
        return blockTypesById;
    }

    // 清空注册表
    void clear() {
        blockTypesById.clear();
        blockTypesByName.clear();
        nextId = 1;
    }
};

// 方块实例（世界中的一个方块）
struct Block {
    uint32_t typeId;

    Block() : typeId(0) {} // 默认为空气
    Block(uint32_t id) : typeId(id) {}

    bool isAir() const {
        return typeId == 0;
    }

    BlockType* getType() const {
        return BlockTypeRegistry::getInstance().getBlockType(typeId);
    }
};

} // namespace game::blocks