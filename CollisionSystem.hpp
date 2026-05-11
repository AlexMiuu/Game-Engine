//
// CollisionSystem.hpp
// Sphere-sphere collision detection for scene objects
//

#ifndef COLLISION_SYSTEM_HPP
#define COLLISION_SYSTEM_HPP

#include "Scene.hpp"
#include "SceneObject.hpp"
#include <glm/glm.hpp>

namespace gps {

    class CollisionSystem {
    public:
        CollisionSystem();
        ~CollisionSystem();

        CollisionSystem(const CollisionSystem&) = delete;
        CollisionSystem& operator=(const CollisionSystem&) = delete;

        void Initialize(Scene* scene);

        /**
         * @brief Check if a moving object collides with any collidable object
         * @param mover The object that just moved to a new position
         * @return true if a collision was detected
         */
        bool CheckCollisions(SceneObject* mover) const;

        /**
         * @brief Like CheckCollisions, but returns the first overlapping object so
         *        callers can compute a contact normal for slide/push response.
         */
        SceneObject* GetCollidingObject(SceneObject* mover) const;

        void SetEnabled(bool enabled) { m_enabled = enabled; }
        bool IsEnabled() const { return m_enabled; }

    private:
        Scene* m_scene;
        bool m_enabled;

        bool SpheresOverlap(const SceneObject& a, const SceneObject& b) const;
    };

} // namespace gps

#endif // COLLISION_SYSTEM_HPP
