//
// CollisionSystem.cpp
// Implementation of sphere-sphere collision detection
//

#include "CollisionSystem.hpp"
#include <iostream>

namespace gps {

    CollisionSystem::CollisionSystem()
        : m_scene(nullptr)
        , m_enabled(true)
    {
    }

    CollisionSystem::~CollisionSystem() {
    }

    void CollisionSystem::Initialize(Scene* scene) {
        m_scene = scene;
        std::cout << "CollisionSystem initialized" << std::endl;
    }

    bool CollisionSystem::CheckCollisions(SceneObject* mover) const {
        return GetCollidingObject(mover) != nullptr;
    }

    SceneObject* CollisionSystem::GetCollidingObject(SceneObject* mover) const {
        if (!m_enabled || !m_scene || !mover) return nullptr;
        if (mover->GetCollisionRadius() <= 0.0f) return nullptr;

        for (const auto& objPtr : m_scene->GetObjects()) {
            SceneObject* other = objPtr.get();

            if (other == mover) continue;
            if (!other->IsActive()) continue;
            if (other->GetCollisionRadius() <= 0.0f) continue;

            if (SpheresOverlap(*mover, *other)) {
                return other;
            }
        }

        return nullptr;
    }

    bool CollisionSystem::SpheresOverlap(const SceneObject& a, const SceneObject& b) const {
        glm::vec3 diff = a.GetWorldCenter() - b.GetWorldCenter();
        float distSq = glm::dot(diff, diff);
        float sumRadii = a.GetCollisionRadius() + b.GetCollisionRadius();
        return distSq < (sumRadii * sumRadii);
    }

} // namespace gps
