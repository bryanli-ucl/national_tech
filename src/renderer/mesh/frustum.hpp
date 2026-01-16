#pragma once

#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace renderer {

/**
 * @brief Plane representation in 3D space
 * Defined by a normal vector and distance from origin
 */
struct Plane {
    glm::vec3 normal; // Plane normal vector
    float distance;   // Distance from origin along normal

    Plane() : normal(0.0f), distance(0.0f) {}

    Plane(const glm::vec3& n, float d) : normal(n), distance(d) {}

    /**
     * @brief Calculate signed distance from point to plane
     * @param point Point in 3D space
     * @return Signed distance (positive = in front, negative = behind)
     */
    float distanceToPoint(const glm::vec3& point) const {
        return glm::dot(normal, point) + distance;
    }
};

/**
 * @brief Axis-Aligned Bounding Box (AABB)
 * Defined by minimum and maximum corners
 */
struct AABB {
    glm::vec3 min; // Minimum corner
    glm::vec3 max; // Maximum corner

    AABB() : min(0.0f), max(0.0f) {}

    AABB(const glm::vec3& minPos, const glm::vec3& maxPos)
    : min(minPos), max(maxPos) {}

    /**
     * @brief Get center point of the bounding box
     * @return Center position
     */
    glm::vec3 getCenter() const {
        return (min + max) * 0.5f;
    }

    /**
     * @brief Get half-extents of the bounding box
     * @return Half-size in each dimension
     */
    glm::vec3 getExtent() const {
        return (max - min) * 0.5f;
    }
};

/**
 * @brief View frustum for frustum culling
 * Consists of 6 planes that define the visible region
 */
class Frustum {
    private:
    // Plane indices for the frustum
    enum {
        PLANE_LEFT = 0,
        PLANE_RIGHT,
        PLANE_BOTTOM,
        PLANE_TOP,
        PLANE_NEAR,
        PLANE_FAR
    };

    std::array<Plane, 6> planes; // Six frustum planes

    public:
    /**
     * @brief Extract frustum planes from view-projection matrix
     * Uses Gribb-Hartmann method to extract planes from matrix
     * @param viewProj Combined view-projection matrix
     */
    void extractFromMatrix(const glm::mat4& viewProj);

    /**
     * @brief Check if an AABB is visible within the frustum
     * Uses the "positive vertex" test for efficient culling
     * @param box Axis-aligned bounding box to test
     * @return true if the box is at least partially visible
     */
    bool isBoxVisible(const AABB& box) const;

    /**
     * @brief Check if a sphere is visible within the frustum
     * @param center Center of the sphere
     * @param radius Radius of the sphere
     * @return true if the sphere is at least partially visible
     */
    bool isSphereVisible(const glm::vec3& center, float radius) const;
};

} // namespace renderer
