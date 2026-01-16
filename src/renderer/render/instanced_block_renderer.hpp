#pragma once

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

#include "renderer/mesh/mesh.hpp"

#include "game/blocks/blocks_types.hpp"

namespace renderer {

// 每个实例的数据
struct BlockInstance {
    glm::vec3 position;
    glm::vec3 rotation; // 欧拉角
    glm::vec3 scale;

    BlockInstance(const glm::vec3& pos = glm::vec3(0.0f))
    : position(pos), rotation(0.0f), scale(1.0f) {}
};

class InstancedBlockRenderer {
    private:
    uint32_t VAO, VBO, EBO;
    uint32_t instanceVBO; // 存储实例数据的VBO
    size_t indexCount;
    size_t instanceCount;
    size_t maxInstances;

    std::vector<glm::mat4> modelMatrices; // 每个实例的模型矩阵

    public:
    InstancedBlockRenderer(const CubeMesh::MeshData& blockMesh, size_t maxInst = 10000)
    : indexCount(blockMesh.indices.size()),
      instanceCount(0),
      maxInstances(maxInst) {

        // 创建基础网格的VAO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        glGenBuffers(1, &instanceVBO);

        glBindVertexArray(VAO);

        // 上传顶点数据
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER,
        blockMesh.vertices.size() * sizeof(Vertex),
        blockMesh.vertices.data(), GL_STATIC_DRAW);

        // 上传索引数据
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        blockMesh.indices.size() * sizeof(uint32_t),
        blockMesh.indices.data(), GL_STATIC_DRAW);

        // 设置顶点属性（位置、法线、纹理坐标）
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, position));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, texCoord));
        glEnableVertexAttribArray(2);

        // 设置实例化属性（模型矩阵）
        // 模型矩阵是4x4，需要4个vec4来存储
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, maxInstances * sizeof(glm::mat4), nullptr, GL_STATIC_DRAW);

        // mat4需要占用4个attribute位置
        for (int i = 0; i < 4; i++) {
            glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE,
            sizeof(glm::mat4), (void*)(sizeof(glm::vec4) * i));
            glEnableVertexAttribArray(3 + i);
            glVertexAttribDivisor(3 + i, 1); // 告诉OpenGL这是实例化属性
        }

        glBindVertexArray(0);

        modelMatrices.reserve(maxInstances);
    }

    ~InstancedBlockRenderer() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteBuffers(1, &instanceVBO);
    }

    // 添加一个实例
    void addInstance(const BlockInstance& instance) {
        if (instanceCount >= maxInstances) {
            throw std::runtime_error("Max instances exceeded");
        }

        // 计算模型矩阵
        glm::mat4 model = glm::mat4(1.0f);
        model           = glm::translate(model, instance.position);
        model           = glm::rotate(model, instance.rotation.x, glm::vec3(1, 0, 0));
        model           = glm::rotate(model, instance.rotation.y, glm::vec3(0, 1, 0));
        model           = glm::rotate(model, instance.rotation.z, glm::vec3(0, 0, 1));
        model           = glm::scale(model, instance.scale);

        modelMatrices.push_back(model);
        instanceCount++;
    }

    // 批量添加实例
    void addInstances(const std::vector<BlockInstance>& instances) {
        for (const auto& inst : instances) {
            addInstance(inst);
        }
    }

    // 更新GPU上的实例数据
    void updateInstanceBuffer() {
        if (instanceCount == 0) return;

        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0,
        instanceCount * sizeof(glm::mat4), modelMatrices.data());
    }

    // 渲染所有实例
    void render() {
        if (instanceCount == 0) return;

        glBindVertexArray(VAO);
        glDrawElementsInstanced(GL_TRIANGLES, indexCount,
        GL_UNSIGNED_INT, 0, instanceCount);
    }

    // 清空所有实例
    void clear() {
        modelMatrices.clear();
        instanceCount = 0;
    }

    size_t getInstanceCount() const { return instanceCount; }
};

} // namespace renderer
