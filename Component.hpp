//
// Component.hpp
// Clasa de baz� pentru sistemul de componente
//

#ifndef COMPONENT_HPP
#define COMPONENT_HPP

namespace gps {
    // Forward declaration pentru a evita circular dependency
    class SceneObject;

    //Base abstract class for all components. Each component will implement its own Update() and OnDestroy() methods.

    class Component {
    public:
        Component() : m_owner(nullptr), m_active(true) {}
        virtual ~Component() = default;

        virtual void Update(float deltaTime);
        virtual void OnDestroy();

        // Getters/Setters
        void SetOwner(SceneObject* owner) { m_owner = owner; }
        SceneObject* GetOwner() const { return m_owner; }

        void SetActive(bool active) { m_active = active; }
        bool IsActive() const { return m_active; }

    protected:
        SceneObject* m_owner;  ///< Owner-ul componentei
        bool m_active;         // if component is active and should be updated

        //utility for fetching the owner as a type
        template<typename T>
        T* GetOwnerAs() const {
            return static_cast<T*>(m_owner);
        }
    };

}

#endif