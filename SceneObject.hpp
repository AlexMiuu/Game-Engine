//
// SceneObject.hpp
// Reprezintã un obiect în scenã cu componente ?i transform
//
#ifndef SCENEOBJECT_HPP
#define SCENEOBJECT_HPP

#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Forward declarations
namespace gps {
    class Component;
    class Model3D;
}

namespace gps {

    /**
     * @brief Clasã simplã pentru transformãri (position, rotation, scale)
     */
    class Transform {
    public:
        Transform()
            : m_position(0.0f)
            , m_rotation(0.0f)
            , m_scale(1.0f)
        {}

        // Getters
        const glm::vec3& GetPosition() const { return m_position; }
        const glm::vec3& GetRotation() const { return m_rotation; }
        const glm::vec3& GetScale() const { return m_scale; }

        // Setters
        void SetPosition(const glm::vec3& pos) { m_position = pos; }
        void SetRotation(const glm::vec3& rot) { m_rotation = rot; }
        void SetScale(const glm::vec3& scale) { m_scale = scale; }

        // Calculeazã matricea de model
        glm::mat4 GetModelMatrix() const {
            glm::mat4 T = glm::translate(glm::mat4(1.0f), m_position);
            glm::mat4 Rx = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.x), glm::vec3(1, 0, 0));
            glm::mat4 Ry = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.y), glm::vec3(0, 1, 0));
            glm::mat4 Rz = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.z), glm::vec3(0, 0, 1));
            glm::mat4 S = glm::scale(glm::mat4(1.0f), m_scale);
            return T * Ry * Rx * Rz * S;
        }

    private:
        glm::vec3 m_position;
        glm::vec3 m_rotation; // Euler angles in degrees
        glm::vec3 m_scale;
    };

    /**
     * @brief Structurã pentru datele de mi?care ale unui obiect
     */
    struct MovementData {
        bool isMoving = false;
        glm::vec3 moveStartPos = glm::vec3(0.0f);
        glm::vec3 moveEndPos = glm::vec3(0.0f);
        float moveStartTime = 0.0f;
        float moveDuration = 1.0f;
    };

    /**
     * @brief SceneObject - container pentru un obiect în scenã
     *
     * Con?ine:
     * - Transform (pozi?ie, rota?ie, scale)
     * - Model 3D pentru rendering
     * - Componente (logicã, comportament)
     * - Bounding sphere (pentru culling, collision)
     * - Date de mi?care
     */
    class SceneObject {
    public:
        SceneObject(int id, const std::string& name = "SceneObject");
        ~SceneObject();

        // Update
        void Update(float deltaTime);

        // Component management
        template<typename T, typename... Args>
        T* AddComponent(Args&&... args) {
            auto component = std::make_unique<T>(std::forward<Args>(args)...);
            T* ptr = component.get();
            component->SetOwner(this);
            m_components.push_back(std::move(component));
            return ptr;
        }

        template<typename T>
        T* GetComponent() {
            for (auto& component : m_components) {
                T* ptr = dynamic_cast<T*>(component.get());
                if (ptr) return ptr;
            }
            return nullptr;
        }

        // Getters
        int GetID() const { return m_id; }
        const std::string& GetName() const { return m_name; }
        bool IsActive() const { return m_active; }

        Transform& GetTransform() { return m_transform; }
        const Transform& GetTransform() const { return m_transform; }

        Model3D* GetModel() const { return m_model; }

        const glm::vec3& GetLocalCenter() const { return m_localCenter; }
        float GetLocalRadius() const { return m_localRadius; }
        const glm::vec3& GetWorldCenter() const { return m_worldCenter; }
        float GetWorldRadius() const { return m_worldRadius; }

        // Setters
        void SetActive(bool active) { m_active = active; }
        void SetModel(Model3D* model) { m_model = model; }
        void SetLocalBounds(const glm::vec3& center, float radius) {
            m_localCenter = center;
            m_localRadius = radius;
        }

        // Update world bounds based on current transform
        void UpdateWorldBounds();

        // Movement data (public pentru acces u?or)
        MovementData movement;

    private:
        int m_id;
        std::string m_name;
        bool m_active;

        Transform m_transform;
        Model3D* m_model; // Pointer cãtre model (nu de?inem ownership-ul)

        // Componente
        std::vector<std::unique_ptr<Component>> m_components;

        // Bounding sphere
        glm::vec3 m_localCenter;  // În local space
        float m_localRadius;
        glm::vec3 m_worldCenter;  // În world space
        float m_worldRadius;
    };

} // namespace gps

#endif // SCENEOBJECT_HPP