#include "frustum.hpp"

namespace renderer {

/**
 * @brief Extract frustum planes from view-projection matrix
 *
 * This implementation uses the Gribb-Hartmann method to extract
 * the six frustum planes directly from the combined view-projection matrix.
 * Each plane is extracted by combining rows of the matrix.
 *
 * @param viewProj Combined view-projection matrix
 */
void Frustum::extractFromMatrix(const glm::mat4& viewProj) {
    // Extract left plane (column 3 + column 0)
    planes[PLANE_LEFT].normal.x = viewProj[0][3] + viewProj[0][0];
    planes[PLANE_LEFT].normal.y = viewProj[1][3] + viewProj[1][0];
    planes[PLANE_LEFT].normal.z = viewProj[2][3] + viewProj[2][0];
    planes[PLANE_LEFT].distance = viewProj[3][3] + viewProj[3][0];

    // Extract right plane (column 3 - column 0)
    planes[PLANE_RIGHT].normal.x = viewProj[0][3] - viewProj[0][0];
    planes[PLANE_RIGHT].normal.y = viewProj[1][3] - viewProj[1][0];
    planes[PLANE_RIGHT].normal.z = viewProj[2][3] - viewProj[2][0];
    planes[PLANE_RIGHT].distance = viewProj[3][3] - viewProj[3][0];

    // Extract bottom plane (column 3 + column 1)
    planes[PLANE_BOTTOM].normal.x = viewProj[0][3] + viewProj[0][1];
    planes[PLANE_BOTTOM].normal.y = viewProj[1][3] + viewProj[1][1];
    planes[PLANE_BOTTOM].normal.z = viewProj[2][3] + viewProj[2][1];
    planes[PLANE_BOTTOM].distance = viewProj[3][3] + viewProj[3][1];

    // Extract top plane (column 3 - column 1)
    planes[PLANE_TOP].normal.x = viewProj[0][3] - viewProj[0][1];
    planes[PLANE_TOP].normal.y = viewProj[1][3] - viewProj[1][1];
    planes[PLANE_TOP].normal.z = viewProj[2][3] - viewProj[2][1];
    planes[PLANE_TOP].distance = viewProj[3][3] - viewProj[3][1];

    // Extract near plane (column 3 + column 2)
    planes[PLANE_NEAR].normal.x = viewProj[0][3] + viewProj[0][2];
    planes[PLANE_NEAR].normal.y = viewProj[1][3] + viewProj[1][2];
    planes[PLANE_NEAR].normal.z = viewProj[2][3] + viewProj[2][2];
    planes[PLANE_NEAR].distance = viewProj[3][3] + viewProj[3][2];

    // Extract far plane (column 3 - column 2)
    planes[PLANE_FAR].normal.x = viewProj[0][3] - viewProj[0][2];
    planes[PLANE_FAR].normal.y = viewProj[1][3] - viewProj[1][2];
    planes[PLANE_FAR].normal.z = viewProj[2][3] - viewProj[2][2];
    planes[PLANE_FAR].distance = viewProj[3][3] - viewProj[3][2];

    // Normalize all planes for accurate distance calculations
    for (auto& plane : planes) {
        float length = glm::length(plane.normal);
        plane.normal /= length;
        plane.distance /= length;
    }
}

/**
 * @brief Check if an AABB is visible within the frustum
 *
 * Uses the "positive vertex" test: for each plane, we find the vertex
 * of the AABB that is furthest in the direction of the plane's normal.
 * If this vertex is outside the plane, the entire box is outside.
 *
 * @param box Axis-aligned bounding box to test
 * @return true if the box is at least partially visible
 */
bool Frustum::isBoxVisible(const AABB& box) const {
    for (const auto& plane : planes) {
        // Find the positive vertex (furthest along plane normal)
        glm::vec3 positiveVertex = box.min;

        // Select max coordinate if normal component is positive
        if (plane.normal.x >= 0) positiveVertex.x = box.max.x;
        if (plane.normal.y >= 0) positiveVertex.y = box.max.y;
        if (plane.normal.z >= 0) positiveVertex.z = box.max.z;

        // If positive vertex is outside this plane, entire box is outside
        if (plane.distanceToPoint(positiveVertex) < 0) {
            return false;
        }
    }

    // Box is at least partially inside all planes
    return true;
}

/**
 * @brief Check if a sphere is visible within the frustum
 *
 * A sphere is outside if its center is more than radius distance
 * behind any frustum plane.
 *
 * @param center Center of the sphere
 * @param radius Radius of the sphere
 * @return true if the sphere is at least partially visible
 */
bool Frustum::isSphereVisible(const glm::vec3& center, float radius) const {
    for (const auto& plane : planes) {
        // If center is further than radius behind plane, sphere is outside
        if (plane.distanceToPoint(center) < -radius) {
            return false;
        }
    }

    // Sphere is at least partially inside all planes
    return true;
}

} // namespace renderer
