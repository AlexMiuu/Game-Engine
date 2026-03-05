//
// Component.hpp
// Clasa de bazã pentru sistemul de componente
//

#ifndef COMPONENT_HPP
#define COMPONENT_HPP

namespace gps {
    // Forward declaration pentru a evita circular dependency
    class SceneObject;

    /**
     * @brief Clasa de bazã abstractã pentru toate componentele
     *
     * Componentele sunt comportamente ata?ate la SceneObject-uri
     * ?i sunt updatate automat de sistemul de scene.
     */
    class Component {
    public:
        Component() : m_owner(nullptr), m_active(true) {}
        virtual ~Component() = default;

        /**
         * @brief Update-ul componentei (apelat în fiecare frame)
         * @param deltaTime Timpul scurs de la ultimul frame
         */
        virtual void Update(float deltaTime);

        /**
         * @brief Cleanup înainte de distrugere
         */
        virtual void OnDestroy();

        // Getters/Setters
        void SetOwner(SceneObject* owner) { m_owner = owner; }
        SceneObject* GetOwner() const { return m_owner; }

        void SetActive(bool active) { m_active = active; }
        bool IsActive() const { return m_active; }

    protected:
        SceneObject* m_owner;  ///< Owner-ul componentei
        bool m_active;         ///< Dacã este activã

        // Helper pentru a ob?ine owner-ul într-un mod type-safe
        template<typename T>
        T* GetOwnerAs() const {
            return static_cast<T*>(m_owner);
        }
    };

} // namespace gps

#endif // COMPONENT_HPP