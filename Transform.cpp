//
// Transform.cpp
// Implementarea clasei Transform
//

#include "Transform.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace gps {

    Transform::Transform()
        : m_position(0.0f)
        , m_rotation(0.0f)
        , m_scale(1.0f)
        , m_modelMatrix(1.0f)
        , m_dirty(true)
    {
    }

    // ===========================
    // POSITION
    // ===========================

    void Transform::SetPosition(const glm::vec3& pos) {
        m_position = pos;
        MarkDirty();
    }

    void Transform::Translate(const glm::vec3& delta) {
        m_position += delta;
        MarkDirty();
    }

    // ===========================
    // ROTATION
    // ===========================

    void Transform::SetRotation(const glm::vec3& rot) {
        m_rotation = rot;
        MarkDirty();
    }

    void Transform::Rotate(const glm::vec3& delta) {
        m_rotation += delta;
        MarkDirty();
    }

    // ===========================
    // SCALE
    // ===========================

    void Transform::SetScale(const glm::vec3& scale) {
        m_scale = scale;
        MarkDirty();
    }

    void Transform::SetScale(float uniformScale) {
        m_scale = glm::vec3(uniformScale);
        MarkDirty();
    }

    // ===========================
    // MODEL MATRIX
    // ===========================

    const glm::mat4& Transform::GetModelMatrix() const {
        if (m_dirty) {
            UpdateMatrix();
            m_dirty = false;
        }
        return m_modelMatrix;
    }

    void Transform::SetModelMatrix(const glm::mat4& matrix) {
        m_modelMatrix = matrix;
        m_dirty = false;

        // Optional: Extract position, rotation, scale din matrix
        // Pentru simplitate, nu facem asta acum
    }

    void Transform::UpdateMatrix() const {
        // Ordinea: Translation * Rotation * Scale
        m_modelMatrix = glm::mat4(1.0f);

        // Translation
        m_modelMatrix = glm::translate(m_modelMatrix, m_position);

        // Rotation (aplicãm în ordinea XYZ)
        m_modelMatrix = glm::rotate(m_modelMatrix, glm::radians(m_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        m_modelMatrix = glm::rotate(m_modelMatrix, glm::radians(m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        m_modelMatrix = glm::rotate(m_modelMatrix, glm::radians(m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

        // Scale
        m_modelMatrix = glm::scale(m_modelMatrix, m_scale);
    }

    // ===========================
    // DIRECTION VECTORS
    // ===========================

    glm::vec3 Transform::GetForward() const {
        const glm::mat4& mat = GetModelMatrix();
        return -glm::normalize(glm::vec3(mat[2]));  // -Z axis
    }

    glm::vec3 Transform::GetRight() const {
        const glm::mat4& mat = GetModelMatrix();
        return glm::normalize(glm::vec3(mat[0]));   // X axis
    }

    glm::vec3 Transform::GetUp() const {
        const glm::mat4& mat = GetModelMatrix();
        return glm::normalize(glm::vec3(mat[1]));   // Y axis
    }

} // namespace gps