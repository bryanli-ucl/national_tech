#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace renderer {

/**
 * @brief Camera movement directions
 */
enum class CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

/**
 * @brief First-person camera class
 *
 * Implements a free-flying first-person camera with:
 * - Keyboard movement (WASD + vertical)
 * - Mouse look (yaw and pitch)
 * - Mouse scroll zoom (FOV adjustment)
 * - Configurable movement speed and mouse sensitivity
 */
class Camera {
    public:
    // Camera vectors
    glm::vec3 position; // Camera position in world space
    glm::vec3 front;    // Forward direction vector
    glm::vec3 up;       // Up direction vector
    glm::vec3 right;    // Right direction vector
    glm::vec3 worldUp;  // World up vector (usually +Y)

    // Euler angles (in degrees)
    float yaw;   // Rotation around Y axis
    float pitch; // Rotation around X axis

    // Camera options
    float movementSpeed;    // Movement speed in units/second
    float mouseSensitivity; // Mouse sensitivity multiplier
    float zoom;             // Field of view in degrees

    /**
     * @brief Construct a new Camera object
     * @param pos Initial position
     * @param up World up vector
     * @param yaw Initial yaw angle in degrees
     * @param pitch Initial pitch angle in degrees
     */
    Camera(
    glm::vec3 pos = glm::vec3(0.0f, 0.0f, 3.0f),
    glm::vec3 up  = glm::vec3(0.0f, 1.0f, 0.0f),
    float yaw     = -90.0f,
    float pitch   = 0.0f);

    /**
     * @brief Get view matrix for rendering
     * @return View matrix (world to camera transform)
     */
    glm::mat4 getViewMatrix() const;

    /**
     * @brief Get projection matrix for rendering
     * @param aspectRatio Viewport aspect ratio (width/height)
     * @param nearPlane Near clipping plane distance
     * @param farPlane Far clipping plane distance
     * @return Perspective projection matrix
     */
    glm::mat4 getProjectionMatrix(
    float aspectRatio,
    float nearPlane = 0.1f,
    float farPlane  = 1024.0f) const;

    /**
     * @brief Process keyboard input for camera movement
     * @param direction Movement direction
     * @param deltaTime Time elapsed since last frame (for frame-rate independence)
     */
    void processKeyboard(CameraMovement direction, float deltaTime);

    /**
     * @brief Process mouse movement for camera rotation
     * @param xoffset Mouse X offset
     * @param yoffset Mouse Y offset
     * @param constrainPitch Whether to limit pitch to prevent flipping
     */
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

    /**
     * @brief Process mouse scroll for zoom (FOV adjustment)
     * @param yoffset Scroll wheel offset
     */
    void processMouseScroll(float yoffset);

    /**
     * @brief Set camera movement speed
     * @param speed New movement speed
     */
    void setMovementSpeed(float speed);

    /**
     * @brief Set mouse sensitivity
     * @param sensitivity New mouse sensitivity
     */
    void setMouseSensitivity(float sensitivity);

    private:
    /**
     * @brief Update camera direction vectors from Euler angles
     * Calculates front, right, and up vectors based on current yaw and pitch
     */
    void updateCameraVectors();
};

} // namespace renderer