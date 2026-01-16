#pragma once

#include "blocks_types.hpp"

namespace game::blocks {

// 方块ID常量
namespace BlockIDs {
constexpr uint32_t AIR    = 0;
constexpr uint32_t GRASS  = 1;
constexpr uint32_t DIRT   = 2;
constexpr uint32_t STONE  = 3;
constexpr uint32_t WOOD   = 4;
constexpr uint32_t LEAVES = 5;
constexpr uint32_t SAND   = 6;
constexpr uint32_t WATER  = 7;
} // namespace BlockIDs

// 初始化所有方块类型
inline void initializeBlockTypes() {
    auto& registry = BlockTypeRegistry::getInstance();

    // 草方块
    registry.registerBlock("grass")
    .setTopSideBottom("grass_carried", "grass_side_carried", "dirt")
    .setHardness(0.6f);

    // 泥土
    registry.registerBlock("dirt")
    .setTexture("dirt")
    .setHardness(0.5f);

    registry.registerBlock("stone")
    .setTexture("stone")
    .setHardness(1.5f);

    registry.registerBlock("wood")
    .setTopSideBottom("pale_oak_log_top", "pale_oak_log_side", "pale_oak_log_top")
    .setHardness(2.0f);

    registry.registerBlock("leaves")
    .setTexture("azalea_leaves")
    .setTransparent(true)
    .setHardness(0.2f);

    registry.registerBlock("sand")
    .setTexture("sand")
    .setHardness(0.5f);
}

} // namespace game::blocks
