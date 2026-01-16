#pragma once

#include <array>
#include <glm/glm.hpp>
#include <vector>

namespace renderer {

// 顶点结构
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

// 立方体面的枚举
enum class CubeFace {
    FRONT, // Z+
    BACK,  // Z-
    LEFT,  // X-
    RIGHT, // X+
    TOP,   // Y+
    BOTTOM // Y-
};

// 立方体网格生成器
class CubeMesh {
    public:
    // 为每个面指定不同的纹理坐标（用于图集）
    struct FaceUV {
        glm::vec2 min; // 左下角
        glm::vec2 max; // 右上角

        FaceUV(float minU, float minV, float maxU, float maxV)
        : min(minU, minV), max(maxU, maxV) {}
    };

    // 创建一个简单的立方体（所有面相同纹理）
    static std::vector<Vertex> createSimple() {
        FaceUV uv(0.0f, 0.0f, 1.0f, 1.0f);
        return createWithAtlas(uv, uv, uv, uv, uv, uv);
    }

    // 创建草方块（使用纹理图集）
    static std::vector<Vertex> createGrassBlock() {
        // 图集布局：左上=顶部, 右上=侧面, 左下=底部
        FaceUV top(0.0f, 0.5f, 0.5f, 1.0f);    // 顶部：草
        FaceUV side(0.5f, 0.5f, 1.0f, 1.0f);   // 侧面：草+土
        FaceUV bottom(0.0f, 0.0f, 0.5f, 0.5f); // 底部：土

        return createWithAtlas(side, side, side, side, top, bottom);
    }

    // 创建带自定义纹理坐标的立方体
    static std::vector<Vertex> createWithAtlas(
    const FaceUV& front,
    const FaceUV& back,
    const FaceUV& left,
    const FaceUV& right,
    const FaceUV& top,
    const FaceUV& bottom) {
        std::vector<Vertex> vertices;
        vertices.reserve(36); // 6 faces * 6 vertices

        // 前面 (Z+)
        addFace(vertices, front,
        { -0.5f, -0.5f, 0.5f }, { 0.5f, -0.5f, 0.5f },
        { 0.5f, 0.5f, 0.5f }, { -0.5f, 0.5f, 0.5f },
        { 0.0f, 0.0f, 1.0f });

        // 后面 (Z-)
        addFace(vertices, back,
        { 0.5f, -0.5f, -0.5f }, { -0.5f, -0.5f, -0.5f },
        { -0.5f, 0.5f, -0.5f }, { 0.5f, 0.5f, -0.5f },
        { 0.0f, 0.0f, -1.0f });

        // 左面 (X-)
        addFace(vertices, left,
        { -0.5f, -0.5f, -0.5f }, { -0.5f, -0.5f, 0.5f },
        { -0.5f, 0.5f, 0.5f }, { -0.5f, 0.5f, -0.5f },
        { -1.0f, 0.0f, 0.0f });

        // 右面 (X+)
        addFace(vertices, right,
        { 0.5f, -0.5f, 0.5f }, { 0.5f, -0.5f, -0.5f },
        { 0.5f, 0.5f, -0.5f }, { 0.5f, 0.5f, 0.5f },
        { 1.0f, 0.0f, 0.0f });

        // 顶面 (Y+)
        addFace(vertices, top,
        { -0.5f, 0.5f, 0.5f }, { 0.5f, 0.5f, 0.5f },
        { 0.5f, 0.5f, -0.5f }, { -0.5f, 0.5f, -0.5f },
        { 0.0f, 1.0f, 0.0f });

        // 底面 (Y-)
        addFace(vertices, bottom,
        { -0.5f, -0.5f, -0.5f }, { 0.5f, -0.5f, -0.5f },
        { 0.5f, -0.5f, 0.5f }, { -0.5f, -0.5f, 0.5f },
        { 0.0f, -1.0f, 0.0f });

        return vertices;
    }

    private:
    // 添加一个四边形面（分解为两个三角形）
    static void addFace(
    std::vector<Vertex>& vertices,
    const FaceUV& uv,
    const glm::vec3& v0,
    const glm::vec3& v1,
    const glm::vec3& v2,
    const glm::vec3& v3,
    const glm::vec3& normal) {
        // 纹理坐标（逆时针）
        std::array<glm::vec2, 4> uvs = {
            glm::vec2(uv.min.x, uv.min.y), // 左下
            glm::vec2(uv.max.x, uv.min.y), // 右下
            glm::vec2(uv.max.x, uv.max.y), // 右上
            glm::vec2(uv.min.x, uv.max.y)  // 左上
        };

        // 第一个三角形 (0, 1, 2)
        vertices.push_back({ v0, normal, uvs[0] });
        vertices.push_back({ v1, normal, uvs[1] });
        vertices.push_back({ v2, normal, uvs[2] });

        // 第二个三角形 (2, 3, 0)
        vertices.push_back({ v2, normal, uvs[2] });
        vertices.push_back({ v3, normal, uvs[3] });
        vertices.push_back({ v0, normal, uvs[0] });
    }
};

} // namespace renderer