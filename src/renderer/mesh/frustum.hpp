#pragma once

#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace renderer {

// 平面表示
struct Plane {
    glm::vec3 normal;
    float distance;

    Plane() : normal(0.0f), distance(0.0f) {}

    Plane(const glm::vec3& n, float d) : normal(n), distance(d) {}

    // 点到平面的距离
    float distanceToPoint(const glm::vec3& point) const {
        return glm::dot(normal, point) + distance;
    }
};

// 轴对齐包围盒（AABB）
struct AABB {
    glm::vec3 min;
    glm::vec3 max;

    AABB() : min(0.0f), max(0.0f) {}
    AABB(const glm::vec3& minPos, const glm::vec3& maxPos)
    : min(minPos), max(maxPos) {}

    glm::vec3 getCenter() const {
        return (min + max) * 0.5f;
    }

    glm::vec3 getExtent() const {
        return (max - min) * 0.5f;
    }
};

// 视锥体
class Frustum {
    private:
    enum {
        PLANE_LEFT = 0,
        PLANE_RIGHT,
        PLANE_BOTTOM,
        PLANE_TOP,
        PLANE_NEAR,
        PLANE_FAR
    };

    std::array<Plane, 6> planes;

    public:
    // 从视图投影矩阵提取视锥体平面
    void extractFromMatrix(const glm::mat4& viewProj) {
        // 左平面
        planes[PLANE_LEFT].normal.x = viewProj[0][3] + viewProj[0][0];
        planes[PLANE_LEFT].normal.y = viewProj[1][3] + viewProj[1][0];
        planes[PLANE_LEFT].normal.z = viewProj[2][3] + viewProj[2][0];
        planes[PLANE_LEFT].distance = viewProj[3][3] + viewProj[3][0];

        // 右平面
        planes[PLANE_RIGHT].normal.x = viewProj[0][3] - viewProj[0][0];
        planes[PLANE_RIGHT].normal.y = viewProj[1][3] - viewProj[1][0];
        planes[PLANE_RIGHT].normal.z = viewProj[2][3] - viewProj[2][0];
        planes[PLANE_RIGHT].distance = viewProj[3][3] - viewProj[3][0];

        // 底平面
        planes[PLANE_BOTTOM].normal.x = viewProj[0][3] + viewProj[0][1];
        planes[PLANE_BOTTOM].normal.y = viewProj[1][3] + viewProj[1][1];
        planes[PLANE_BOTTOM].normal.z = viewProj[2][3] + viewProj[2][1];
        planes[PLANE_BOTTOM].distance = viewProj[3][3] + viewProj[3][1];

        // 顶平面
        planes[PLANE_TOP].normal.x = viewProj[0][3] - viewProj[0][1];
        planes[PLANE_TOP].normal.y = viewProj[1][3] - viewProj[1][1];
        planes[PLANE_TOP].normal.z = viewProj[2][3] - viewProj[2][1];
        planes[PLANE_TOP].distance = viewProj[3][3] - viewProj[3][1];

        // 近平面
        planes[PLANE_NEAR].normal.x = viewProj[0][3] + viewProj[0][2];
        planes[PLANE_NEAR].normal.y = viewProj[1][3] + viewProj[1][2];
        planes[PLANE_NEAR].normal.z = viewProj[2][3] + viewProj[2][2];
        planes[PLANE_NEAR].distance = viewProj[3][3] + viewProj[3][2];

        // 远平面
        planes[PLANE_FAR].normal.x = viewProj[0][3] - viewProj[0][2];
        planes[PLANE_FAR].normal.y = viewProj[1][3] - viewProj[1][2];
        planes[PLANE_FAR].normal.z = viewProj[2][3] - viewProj[2][2];
        planes[PLANE_FAR].distance = viewProj[3][3] - viewProj[3][2];

        // 归一化所有平面
        for (auto& plane : planes) {
            float length = glm::length(plane.normal);
            plane.normal /= length;
            plane.distance /= length;
        }
    }

    // 检查AABB是否在视锥体内
    bool isBoxVisible(const AABB& box) const {
        for (const auto& plane : planes) {
            // 计算AABB的正向顶点（相对于平面法线）
            glm::vec3 positiveVertex = box.min;

            if (plane.normal.x >= 0) positiveVertex.x = box.max.x;
            if (plane.normal.y >= 0) positiveVertex.y = box.max.y;
            if (plane.normal.z >= 0) positiveVertex.z = box.max.z;

            // 如果正向顶点在平面外侧，整个盒子在外侧
            if (plane.distanceToPoint(positiveVertex) < 0) {
                return false;
            }
        }

        return true;
    }

    // 检查球体是否在视锥体内
    bool isSphereVisible(const glm::vec3& center, float radius) const {
        for (const auto& plane : planes) {
            if (plane.distanceToPoint(center) < -radius) {
                return false;
            }
        }
        return true;
    }
};

} // namespace renderer
