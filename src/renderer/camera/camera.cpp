#include "camera.hpp"

namespace renderer {

/**
 * @brief Construct a new Camera object with default settings
 * @param pos Initial camera position
 * @param up World up vector (typically +Y axis)
 * @param yaw Initial yaw angle in degrees (-90° looks along -Z)
 * @param pitch Initial pitch angle in degrees (0° looks horizontally)
 */
Camera::Camera(glm::vec3 pos, glm::vec3 up, float yaw, float pitch)
: position(pos),
  front(glm::vec3(0.0f, 0.0f, -1.0f)),
  worldUp(up),
  yaw(yaw),
  pitch(pitch),
  movementSpeed(8.0f),
  mouseSensitivity(0.1f),
  zoom(45.0f) {
    // Calculate initial camera vectors
    updateCameraVectors();
}

/**
 * @brief Get the view matrix for transforming from world space to camera space
 * @return 4x4 view matrix using look-at transformation
 */
glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, position + front, up);
}

/**
 * @brief Get the perspective projection matrix
 * @param aspectRatio Viewport width/height ratio
 * @param nearPlane Near clipping plane (objects closer are culled)
 * @param farPlane Far clipping plane (objects further are culled)
 * @return 4x4 perspective projection matrix
 */
glm::mat4 Camera::getProjectionMatrix(float aspectRatio, float nearPlane, float farPlane) const {
    return glm::perspective(glm::radians(zoom), aspectRatio, nearPlane, farPlane);
}

/**
 * @brief Process keyboard input for camera movement
 *
 * Moves the camera in the specified direction at a speed proportional
 * to deltaTime for frame-rate independent movement.
 *
 * @param direction Direction to move (FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN)
 * @param deltaTime Time elapsed since last frame in seconds
 */
void Camera::processKeyboard(CameraMovement direction, float deltaTime) {
    float velocity = movementSpeed * deltaTime;

    switch (direction) {
    case CameraMovement::FORWARD:
        position += front * velocity;
        break;
    case CameraMovement::BACKWARD:
        position -= front * velocity;
        break;
    case CameraMovement::LEFT:
        position -= right * velocity;
        break;
    case CameraMovement::RIGHT:
        position += right * velocity;
        break;
    case CameraMovement::UP:
        position += up * velocity;
        break;
    case CameraMovement::DOWN:
        position -= up * velocity;
        break;
    }
}

/**
 * @brief Process mouse movement for camera rotation
 *
 * Updates yaw and pitch based on mouse offset, with optional
 * pitch clamping to prevent camera flipping.
 *
 * @param xoffset Mouse movement in X direction
 * @param yoffset Mouse movement in Y direction
 * @param constrainPitch If true, clamp pitch to [-89°, 89°]
 */
void Camera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    // Apply mouse sensitivity
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    // Update angles
    yaw += xoffset;
    pitch += yoffset;

    // Constrain pitch to prevent screen flipping
    if (constrainPitch) {
        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;
    }

    // Recalculate camera vectors
    updateCameraVectors();
}

/**
 * @brief Process mouse scroll for zoom (field of view adjustment)
 *
 * Decreases FOV to zoom in, increases to zoom out.
 * Clamped between 1° and 45°.
 *
 * @param yoffset Scroll wheel offset (positive = scroll up = zoom in)
 */
void Camera::processMouseScroll(float yoffset) {
    zoom -= yoffset;

    // Clamp zoom to reasonable range
    if (zoom < 1.0f)
        zoom = 1.0f;
    if (zoom > 45.0f)
        zoom = 45.0f;
}

/**
 * @brief Set camera movement speed
 * @param speed Movement speed in units per second
 */
void Camera::setMovementSpeed(float speed) {
    movementSpeed = speed;
}

/**
 * @brief Set mouse sensitivity multiplier
 * @param sensitivity Mouse sensitivity (higher = more sensitive)
 */
void Camera::setMouseSensitivity(float sensitivity) {
    mouseSensitivity = sensitivity;
}

/**
 * @brief Update camera direction vectors from Euler angles
 *
 * Converts yaw and pitch angles into normalized direction vectors:
 * - front: forward direction (from yaw and pitch)
 * - right: right direction (perpendicular to front and worldUp)
 * - up: camera up direction (perpendicular to right and front)
 */
void Camera::updateCameraVectors() {
    // Calculate new front vector from Euler angles
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front      = glm::normalize(newFront);

    // Recalculate right and up vectors
    // Right vector is perpendicular to front and world up
    right = glm::normalize(glm::cross(front, worldUp));

    // Up vector is perpendicular to right and front
    up = glm::normalize(glm::cross(right, front));
}

} // namespace renderer
