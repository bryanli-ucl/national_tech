#pragma once

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


namespace renderer {

enum class CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class Camera {
    public:
    // 相机属性
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    // 欧拉角
    float yaw;
    float pitch;

    // 相机选项
    float movementSpeed;
    float mouseSensitivity;
    float zoom;

    // 构造函数
    Camera(
    glm::vec3 pos = glm::vec3(0.0f, 0.0f, 3.0f),
    glm::vec3 up  = glm::vec3(0.0f, 1.0f, 0.0f),
    float yaw     = -90.0f,
    float pitch   = 0.0f)
    : position(pos),
      front(glm::vec3(0.0f, 0.0f, -1.0f)),
      worldUp(up),
      yaw(yaw), pitch(pitch),
      movementSpeed(2.5f),
      mouseSensitivity(0.1f),
      zoom(45.0f) {
        updateCameraVectors();
    }

    // 获取视图矩阵
    glm::mat4 getViewMatrix() const {
        return glm::lookAt(position, position + front, up);
    }

    // 获取投影矩阵
    glm::mat4 getProjectionMatrix(float aspectRatio, float nearPlane = 0.1f, float farPlane = 100.0f) const {
        return glm::perspective(glm::radians(zoom), aspectRatio, nearPlane, farPlane);
    }

    // 处理键盘输入
    void processKeyboard(CameraMovement direction, float deltaTime) {
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

    // 处理鼠标移动
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) {
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        yaw += xoffset;
        pitch += yoffset;

        // 限制俯仰角，防止屏幕翻转
        if (constrainPitch) {
            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;
        }

        updateCameraVectors();
    }

    // 处理鼠标滚轮（缩放）
    void processMouseScroll(float yoffset) {
        zoom -= yoffset;
        if (zoom < 1.0f)
            zoom = 1.0f;
        if (zoom > 45.0f)
            zoom = 45.0f;
    }

    // 设置移动速度
    void setMovementSpeed(float speed) {
        movementSpeed = speed;
    }

    // 设置鼠标灵敏度
    void setMouseSensitivity(float sensitivity) {
        mouseSensitivity = sensitivity;
    }

    private:
    // 更新相机向量
    void updateCameraVectors() {
        // 计算新的front向量
        glm::vec3 newFront;
        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        newFront.y = sin(glm::radians(pitch));
        newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front      = glm::normalize(newFront);

        // 重新计算right和up向量
        right = glm::normalize(glm::cross(front, worldUp));
        up    = glm::normalize(glm::cross(right, front));
    }
};

} // namespace renderer
