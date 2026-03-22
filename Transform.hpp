//
// Transform.hpp
// Gestioneaz� transform�rile unui obiect (position, rotation, scale)
//

#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace gps {

    /**
     * @brief Clas� pentru transform�ri 3D
     *
     * �nlocuie?te: glm::mat4 modelMatrix din main.cpp
     *
     * Exemplu:
     * @code
     * Transform transform;
     * transform.SetPosition(glm::vec3(10, 0, 5));
     * transform.Rotate(glm::vec3(0, 45, 0));
     * glm::mat4 matrix = transform.GetModelMatrix();
     * @endcode
     */
    class Transform {
    public:
        Transform();

        // ===========================
        // POSITION
        // ===========================

        const glm::vec3& GetPosition() const { return m_position; }
        void SetPosition(const glm::vec3& pos);
        void SetHeight(float height);
        void Translate(const glm::vec3& delta);

        // ===========================
        // ROTATION (Euler angles �n grade)
        // ===========================

        const glm::vec3& GetRotation() const { return m_rotation; }
        void SetRotation(const glm::vec3& rot);
        void Rotate(const glm::vec3& delta);

        // ===========================
        // SCALE
        // ===========================

        const glm::vec3& GetScale() const { return m_scale; }
        void SetScale(const glm::vec3& scale);
        void SetScale(float uniformScale);

        // ===========================
        // MODEL MATRIX
        // ===========================

        /**
         * @brief Ob?ine model matrix (Position * Rotation * Scale)
         * Matrix-ul e cached ?i recalculat doar c�nd e nevoie
         */
        const glm::mat4& GetModelMatrix() const;

        /**
         * @brief Seteaz� direct model matrix-ul (bypass T*R*S)
         */
        void SetModelMatrix(const glm::mat4& matrix);

        // ===========================
        // DIRECTION VECTORS
        // ===========================

        glm::vec3 GetForward() const;
        glm::vec3 GetRight() const;
        glm::vec3 GetUp() const;

    private:
        glm::vec3 m_position;
        glm::vec3 m_rotation;  // Euler angles (X, Y, Z) �n grade
        glm::vec3 m_scale;

        mutable glm::mat4 m_modelMatrix;
        mutable bool m_dirty;  // Flag pentru lazy update

        void MarkDirty() { m_dirty = true; }
        void UpdateMatrix() const;
    };

} // namespace gps

#endif // TRANSFORM_HPP