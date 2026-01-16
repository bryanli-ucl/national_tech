#pragma once

#include <glm/glm.hpp>

#include <array>
#include <vector>

#include "game/blocks/blocks_types.hpp"
#include "game/chuck/chunk_mesh_optimizer.hpp"

#include "renderer/mesh/mesh.hpp"
#include "renderer/texture/texture.hpp"


namespace game::chuck {

/**
 * 贪婪网格生成器
 *
 * 算法流程：
 * 1. 为每个方向（6个面）生成一个2D切片遮罩
 * 2. 在遮罩中查找可以合并的矩形区域
 * 3. 生成合并后的大矩形面
 *
 * 优势：
 * - 减少 80-95% 的顶点数量
 * - 减少绘制调用
 * - 提高缓存友好性
 */
class GreedyMesher {
    public:
    // 构造函数
    explicit GreedyMesher(renderer::TextureAtlas* textureAtlas)
    : atlas(textureAtlas) {
        if (!atlas) {
            throw std::runtime_error("TextureAtlas cannot be null");
        }
    }

    // 主入口：生成区块的贪婪网格
    std::unordered_map<uint32_t, renderer::CubeMesh::MeshData> generateMesh(
    const VoxelChunk& chunk) {

        meshes.clear();

        int sizeX = chunk.getSizeX();
        int sizeY = chunk.getSizeY();
        int sizeZ = chunk.getSizeZ();

        // 为6个方向分别生成网格
        // X轴方向
        generateAxisMesh(chunk, Axis::X, Direction::POSITIVE, sizeX, sizeY, sizeZ);
        generateAxisMesh(chunk, Axis::X, Direction::NEGATIVE, sizeX, sizeY, sizeZ);

        // Y轴方向
        generateAxisMesh(chunk, Axis::Y, Direction::POSITIVE, sizeX, sizeY, sizeZ);
        generateAxisMesh(chunk, Axis::Y, Direction::NEGATIVE, sizeX, sizeY, sizeZ);

        // Z轴方向
        generateAxisMesh(chunk, Axis::Z, Direction::POSITIVE, sizeX, sizeY, sizeZ);
        generateAxisMesh(chunk, Axis::Z, Direction::NEGATIVE, sizeX, sizeY, sizeZ);

        return std::move(meshes);
    }

    private:
    renderer::TextureAtlas* atlas;
    std::unordered_map<uint32_t, renderer::CubeMesh::MeshData> meshes;

    // 坐标轴枚举
    enum class Axis { X,
        Y,
        Z };
    enum class Direction { POSITIVE,
        NEGATIVE };

    // 遮罩条目：存储方块类型和是否可见
    struct MaskEntry {
        uint32_t blockType;
        bool visible;

        MaskEntry() : blockType(0), visible(false) {}
        MaskEntry(uint32_t type, bool vis) : blockType(type), visible(vis) {}

        bool operator==(const MaskEntry& other) const {
            return blockType == other.blockType && visible == other.visible;
        }

        bool operator!=(const MaskEntry& other) const {
            return !(*this == other);
        }

        bool isEmpty() const {
            return !visible || blockType == 0;
        }
    };

    /**
     * 为指定轴向生成网格
     */
    void generateAxisMesh(const VoxelChunk& chunk,
    Axis axis,
    Direction direction,
    int sizeX,
    int sizeY,
    int sizeZ) {

        // 根据轴向确定遍历的维度
        int depth, width, height;
        getAxisDimensions(axis, sizeX, sizeY, sizeZ, depth, width, height);

        // 创建2D遮罩（width x height）
        std::vector<MaskEntry> mask(width * height);

        // 遍历深度维度（沿着法线方向）
        for (int d = 0; d < depth; d++) {
            // 1. 生成当前切片的遮罩
            generateSliceMask(chunk, axis, direction, d, width, height, mask);

            // 2. 从遮罩生成合并的矩形
            generateQuadsFromMask(mask, width, height, axis, direction, d);

            // 3. 清空遮罩准备下一个切片
            std::fill(mask.begin(), mask.end(), MaskEntry());
        }
    }

    /**
     * 生成单个切片的遮罩
     */
    void generateSliceMask(const VoxelChunk& chunk,
    Axis axis,
    Direction direction,
    int depth,
    int width,
    int height,
    std::vector<MaskEntry>& mask) {

        for (int h = 0; h < height; h++) {
            for (int w = 0; w < width; w++) {
                // 获取3D坐标
                glm::ivec3 pos         = get3DPosition(axis, w, h, depth);
                glm::ivec3 neighborPos = pos + getNormalOffset(axis, direction);

                // 获取当前方块和邻居方块
                uint32_t currentBlock  = chunk.getBlock(pos.x, pos.y, pos.z);
                uint32_t neighborBlock = chunk.getBlock(neighborPos.x, neighborPos.y, neighborPos.z);

                // 判断是否需要渲染这个面
                bool shouldRender = shouldRenderFace(chunk, currentBlock, neighborBlock,
                neighborPos);

                if (shouldRender) {
                    mask[h * width + w] = MaskEntry(currentBlock, true);
                } else {
                    mask[h * width + w] = MaskEntry(0, false);
                }
            }
        }
    }

    /**
     * 从遮罩生成合并的矩形
     *
     * 算法：
     * 1. 扫描遮罩，找到第一个非空条目
     * 2. 向右扩展，找到最大宽度
     * 3. 向下扩展，找到最大高度
     * 4. 生成矩形，标记已处理的区域
     */
    void generateQuadsFromMask(std::vector<MaskEntry>& mask,
    int width,
    int height,
    Axis axis,
    Direction direction,
    int depth) {

        for (int h = 0; h < height; h++) {
            for (int w = 0; w < width;) {
                MaskEntry& entry = mask[h * width + w];

                // 跳过空条目
                if (entry.isEmpty()) {
                    w++;
                    continue;
                }

                // 记录当前方块类型
                uint32_t blockType = entry.blockType;

                // 1. 计算矩形宽度（向右扩展）
                int rectWidth = 1;
                while (w + rectWidth < width) {
                    MaskEntry& nextEntry = mask[h * width + w + rectWidth];
                    if (nextEntry != entry) {
                        break;
                    }
                    rectWidth++;
                }

                // 2. 计算矩形高度（向下扩展）
                int rectHeight       = 1;
                bool canExtendHeight = true;

                while (h + rectHeight < height && canExtendHeight) {
                    // 检查下一行的所有宽度是否都匹配
                    for (int checkW = 0; checkW < rectWidth; checkW++) {
                        MaskEntry& checkEntry = mask[(h + rectHeight) * width + w + checkW];
                        if (checkEntry != entry) {
                            canExtendHeight = false;
                            break;
                        }
                    }

                    if (canExtendHeight) {
                        rectHeight++;
                    }
                }

                // 3. 生成合并的矩形面
                blocks::BlockFace face = getFaceFromAxisDirection(axis, direction);
                createMergedQuad(blockType, axis, direction, depth,
                w, h, rectWidth, rectHeight, face);

                // 4. 清除遮罩中已处理的区域
                for (int clearH = 0; clearH < rectHeight; clearH++) {
                    for (int clearW = 0; clearW < rectWidth; clearW++) {
                        mask[(h + clearH) * width + w + clearW] = MaskEntry();
                    }
                }

                // 继续处理下一个位置
                w += rectWidth;
            }
        }
    }

    /**
     * 创建合并的矩形面
     */
    void createMergedQuad(uint32_t blockType,
    Axis axis,
    Direction direction,
    int depth,
    int startW,
    int startH,
    int width,
    int height,
    blocks::BlockFace face) {

        // 获取方块类型定义
        auto* blockTypeDef = blocks::BlockTypeRegistry::getInstance().getBlockType(blockType);
        if (!blockTypeDef) return;

        // 获取纹理坐标
        const std::string& textureName = blockTypeDef->getTexture(face);
        renderer::TextureUV uv         = atlas->getUV(textureName);

        // 计算4个顶点的3D位置
        std::array<glm::vec3, 4> vertices = calculateQuadVertices(
        axis, direction, depth, startW, startH, width, height);

        // 计算法线
        glm::vec3 normal = calculateNormal(axis, direction);

        // 计算UV坐标（支持纹理平铺）
        std::array<glm::vec2, 4> uvCoords = calculateUVCoords(uv, width, height);

        // 添加到网格数据
        addQuadToMesh(meshes[blockType], vertices, normal, uvCoords, uv);
    }

    /**
     * 计算矩形的4个顶点位置
     */
    std::array<glm::vec3, 4> calculateQuadVertices(Axis axis,
    Direction direction,
    int depth,
    int startW,
    int startH,
    int width,
    int height) {
        std::array<glm::vec3, 4> vertices;

        // 基础偏移（用于正/负方向）
        float depthOffset = (direction == Direction::POSITIVE) ? 1.0f : 0.0f;

        switch (axis) {
        case Axis::X:
            // X轴：YZ平面
            vertices[0] = glm::vec3(depth + depthOffset, startH, startW);
            vertices[1] = glm::vec3(depth + depthOffset, startH, startW + width);
            vertices[2] = glm::vec3(depth + depthOffset, startH + height, startW + width);
            vertices[3] = glm::vec3(depth + depthOffset, startH + height, startW);
            break;

        case Axis::Y:
            // Y轴：XZ平面
            vertices[0] = glm::vec3(startW, depth + depthOffset, startH);
            vertices[1] = glm::vec3(startW + width, depth + depthOffset, startH);
            vertices[2] = glm::vec3(startW + width, depth + depthOffset, startH + height);
            vertices[3] = glm::vec3(startW, depth + depthOffset, startH + height);
            break;

        case Axis::Z:
            // Z轴：XY平面
            vertices[0] = glm::vec3(startW, startH, depth + depthOffset);
            vertices[1] = glm::vec3(startW + width, startH, depth + depthOffset);
            vertices[2] = glm::vec3(startW + width, startH + height, depth + depthOffset);
            vertices[3] = glm::vec3(startW, startH + height, depth + depthOffset);
            break;
        }

        // 根据方向调整顶点顺序（确保逆时针）
        if (direction == Direction::NEGATIVE) {
            std::swap(vertices[1], vertices[3]);
        }

        return vertices;
    }

    /**
     * 计算UV坐标（支持纹理平铺）
     */
    std::array<glm::vec2, 4> calculateUVCoords(const renderer::TextureUV& baseUV,
    int width,
    int height) {
        std::array<glm::vec2, 4> uvCoords;

        float uvWidth  = baseUV.max.x - baseUV.min.x;
        float uvHeight = baseUV.max.y - baseUV.min.y;

        // 添加小量内缩以避免纹理边缘渗色
        // 对于图集纹理，即使使用 CLAMP_TO_EDGE，由于 mipmap 和浮点精度，
        // 仍可能采样到相邻纹理。内缩半个像素可以避免此问题
        constexpr float INSET_EPSILON = 0.0005f; // 约为半个像素（假设2048图集）

        float insetMinU = baseUV.min.x + INSET_EPSILON;
        float insetMinV = baseUV.min.y + INSET_EPSILON;
        float insetMaxU = baseUV.max.x - INSET_EPSILON;
        float insetMaxV = baseUV.max.y - INSET_EPSILON;

        // 计算内缩后的UV范围
        float insetWidth = insetMaxU - insetMinU;
        float insetHeight = insetMaxV - insetMinV;

        // UV坐标平铺，但限制在内缩的边界内
        uvCoords[0] = glm::vec2(insetMinU, insetMinV);
        uvCoords[1] = glm::vec2(insetMinU + insetWidth * width, insetMinV);
        uvCoords[2] = glm::vec2(insetMinU + insetWidth * width, insetMinV + insetHeight * height);
        uvCoords[3] = glm::vec2(insetMinU, insetMinV + insetHeight * height);

        return uvCoords;
    }

    /**
     * 计算法线向量
     */
    glm::vec3 calculateNormal(Axis axis, Direction direction) {
        float sign = (direction == Direction::POSITIVE) ? 1.0f : -1.0f;

        switch (axis) {
        case Axis::X: return glm::vec3(sign, 0, 0);
        case Axis::Y: return glm::vec3(0, sign, 0);
        case Axis::Z: return glm::vec3(0, 0, sign);
        }

        return glm::vec3(0, 1, 0); // 默认
    }

    /**
     * 添加矩形到网格数据
     */
    void addQuadToMesh(renderer::CubeMesh::MeshData& meshData,
    const std::array<glm::vec3, 4>& vertices,
    const glm::vec3& normal,
    const std::array<glm::vec2, 4>& uvCoords,
    const renderer::TextureUV& textureBounds) { // 新增参数

        uint32_t baseIndex = meshData.vertices.size();

        // 将纹理边界打包成 vec4
        glm::vec4 bounds(
        textureBounds.min.x,
        textureBounds.min.y,
        textureBounds.max.x,
        textureBounds.max.y);

        // 添加4个顶点
        for (int i = 0; i < 4; i++) {
            renderer::Vertex vertex;
            vertex.position      = vertices[i];
            vertex.normal        = normal;
            vertex.texCoord      = uvCoords[i];
            vertex.textureBounds = bounds; // 设置纹理边界
            meshData.vertices.push_back(vertex);
        }

        // 添加索引（不变）
        meshData.indices.push_back(baseIndex + 0);
        meshData.indices.push_back(baseIndex + 1);
        meshData.indices.push_back(baseIndex + 2);

        meshData.indices.push_back(baseIndex + 0);
        meshData.indices.push_back(baseIndex + 2);
        meshData.indices.push_back(baseIndex + 3);
    }

    // ========== 辅助函数 ==========

    /**
     * 根据轴向获取维度大小
     */
    void getAxisDimensions(Axis axis, int sizeX, int sizeY, int sizeZ, int& depth, int& width, int& height) {
        switch (axis) {
        case Axis::X:
            depth  = sizeX;
            width  = sizeZ;
            height = sizeY;
            break;
        case Axis::Y:
            depth  = sizeY;
            width  = sizeX;
            height = sizeZ;
            break;
        case Axis::Z:
            depth  = sizeZ;
            width  = sizeX;
            height = sizeY;
            break;
        }
    }

    /**
     * 从2D坐标转换为3D坐标
     */
    glm::ivec3 get3DPosition(Axis axis, int w, int h, int d) {
        switch (axis) {
        case Axis::X: return glm::ivec3(d, h, w);
        case Axis::Y: return glm::ivec3(w, d, h);
        case Axis::Z: return glm::ivec3(w, h, d);
        }
        return glm::ivec3(0);
    }

    /**
     * 获取法线方向的偏移
     */
    glm::ivec3 getNormalOffset(Axis axis, Direction direction) {
        int sign = (direction == Direction::POSITIVE) ? 1 : -1;

        switch (axis) {
        case Axis::X: return glm::ivec3(sign, 0, 0);
        case Axis::Y: return glm::ivec3(0, sign, 0);
        case Axis::Z: return glm::ivec3(0, 0, sign);
        }

        return glm::ivec3(0);
    }

    /**
     * 从轴向和方向获取BlockFace
     */
    blocks::BlockFace getFaceFromAxisDirection(Axis axis, Direction direction) {
        using namespace blocks;

        if (axis == Axis::X) {
            return (direction == Direction::POSITIVE) ? BlockFace::RIGHT : BlockFace::LEFT;
        } else if (axis == Axis::Y) {
            return (direction == Direction::POSITIVE) ? BlockFace::TOP : BlockFace::BOTTOM;
        } else {
            return (direction == Direction::POSITIVE) ? BlockFace::FRONT : BlockFace::BACK;
        }
    }

    /**
     * 判断是否应该渲染面
     */
    bool shouldRenderFace(const VoxelChunk& chunk,
    uint32_t currentBlock,
    uint32_t neighborBlock,
    const glm::ivec3& neighborPos) {
        (void)neighborBlock;

        // 当前方块是空气，不渲染
        if (currentBlock == 0) return false;


        // 邻居是实心方块，不渲染（被遮挡）
        if (chunk.isBlockSolid(neighborPos.x, neighborPos.y, neighborPos.z)) {
            return false;
        }

        // 其他情况渲染
        return true;
    }
};

} // namespace game::chuck
