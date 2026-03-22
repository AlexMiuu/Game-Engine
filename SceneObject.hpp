//
// SceneObject.hpp
// Reprezint� un obiect �n scen� cu componente ?i transform
//
#ifndef SCENEOBJECT_HPP
#define SCENEOBJECT_HPP

#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "Transform.hpp"

// Forward declarations
namespace gps {
    class Component;
    class Model3D;
}

namespace gps {

    /**
     * @brief Structur� pentru datele de mi?care ale unui obiect
     */
    struct MovementData {
        bool isMoving = false;
        glm::vec3 moveStartPos = glm::vec3(0.0f);
        glm::vec3 moveEndPos = glm::vec3(0.0f);
        glm::vec3 moveDirection = glm::vec3(0.0f, 0.0f, -1.0f);
        float moveStartTime = 0.0f;
        float moveDuration = 1.0f;
    };


    struct UnitStats {
        int maxHealth = 100;
        int health = 100;
        int attack = 10;
        int attackRange = 5;
        bool isCombatUnit = false;
        bool isAlive = true;

        // Runtime combat state (managed by CombatSystem)
        float attackCooldown = 0.0f;
        int   targetID = -1;

        // Resource production
        std::string resourceType = "none";
        float productionRate = 0.0f;
    };
    /**
     * @brief SceneObject - container pentru un obiect �n scen�
     *
     * Con?ine:
     * - Transform (pozi?ie, rota?ie, scale)
     * - Model 3D pentru rendering
     * - Componente (logic�, comportament)
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
        const std::string& GetTag() const { return m_tag; }
        bool IsActive() const { return m_active; }

        Transform& GetTransform() { return m_transform; }
        const Transform& GetTransform() const { return m_transform; }

        Model3D* GetModel() const { return m_model; }

        const glm::vec3& GetLocalCenter() const { return m_localCenter; }
        float GetLocalRadius() const { return m_localRadius; }
        const glm::vec3& GetWorldCenter() const { return m_worldCenter; }
        float GetWorldRadius() const { return m_worldRadius; }

        float GetCollisionRadius() const { return m_collisionRadius; }
        void SetCollisionRadius(float radius) { m_collisionRadius = radius; }
        bool IsCollidable() const { return m_collisionRadius > 0.0f; }

        // Setters
        void SetActive(bool active) { m_active = active; }
        void SetModel(Model3D* model) { m_model = model; }
        void SetLocalBounds(const glm::vec3& center, float radius) {
            m_localCenter = center;
            m_localRadius = radius;
        }
        void SetTag(const std::string& tag) { m_tag = tag; }

        // Update world bounds based on current transform
        void UpdateWorldBounds();

        // Movement data (public pentru acces u?or)
        MovementData movement;

        // Unit stats (public for easy access, like MovementData)
        UnitStats unitStats;

    private:
        int m_id;
        std::string m_name;
        std:: string m_tag;
        bool m_active;

        Transform m_transform;
        Model3D* m_model; // Pointer c�tre model (nu de?inem ownership-ul)

        // Componente
        std::vector<std::unique_ptr<Component>> m_components;

        // Bounding sphere
        glm::vec3 m_localCenter;  // �n local space
        float m_localRadius;
        glm::vec3 m_worldCenter;  // �n world space
        float m_worldRadius;
        float m_collisionRadius;  // 0.0f = not collidable
    };

} // namespace gps

#endif // SCENEOBJECT_HPP