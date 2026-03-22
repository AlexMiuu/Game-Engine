//
// CombatSystem.cpp
//

#include "CombatSystem.hpp"
#include <glm/glm.hpp>
#include <iostream>

namespace gps {

    CombatSystem::CombatSystem() = default;
    CombatSystem::~CombatSystem() = default;

    void CombatSystem::Initialize(Scene* scene) {
        m_scene = scene;
        std::cout << "CombatSystem initialized" << std::endl;
    }

    int CombatSystem::FindNearestEnemy(SceneObject* attacker) const {
        const UnitStats& atkStats = attacker->unitStats;
        float bestDistSq = atkStats.attackRange * atkStats.attackRange;
        int bestID = -1;

        for (const auto& obj : m_scene->GetObjects()) {
            SceneObject* candidate = obj.get();
            if (!candidate->IsActive()) continue;
            if (candidate->GetID() == attacker->GetID()) continue;

            const UnitStats& cStats = candidate->unitStats;
            if (!cStats.isCombatUnit || !cStats.isAlive) continue;

            // Simple faction check: enemy = different name prefix
            // Both are combat units — they fight each other
            // (Extend with faction field if needed later)
            glm::vec3 diff = candidate->GetWorldCenter() - attacker->GetWorldCenter();
            float distSq = glm::dot(diff, diff);

            if (distSq < bestDistSq) {
                bestDistSq = distSq;
                bestID = candidate->GetID();
            }
        }

        return bestID;
    }

    void CombatSystem::Update(float deltaTime) {
        if (!m_scene || !m_enabled) return;

        const auto& objects = m_scene->GetObjects();

        for (const auto& obj : objects) {
            SceneObject* attacker = obj.get();
            if (!attacker->IsActive()) continue;

            UnitStats& stats = attacker->unitStats;
            if (!stats.isCombatUnit || !stats.isAlive) continue;

            // Tick attack cooldown
            if (stats.attackCooldown > 0.0f)
                stats.attackCooldown -= deltaTime;

            // Validate current target
            if (stats.targetID != -1) {
                SceneObject* target = m_scene->GetObjectByID(stats.targetID);
                if (!target || !target->IsActive() || !target->unitStats.isAlive) {
                    stats.targetID = -1;
                } else {
                    // Check still in range
                    glm::vec3 diff = target->GetWorldCenter() - attacker->GetWorldCenter();
                    float distSq = glm::dot(diff, diff);
                    float range = static_cast<float>(stats.attackRange);
                    if (distSq > range * range)
                        stats.targetID = -1;
                }
            }

            // Find new target if none
            if (stats.targetID == -1)
                stats.targetID = FindNearestEnemy(attacker);

            // Attack if ready and target exists
            if (stats.targetID != -1 && stats.attackCooldown <= 0.0f) {
                SceneObject* target = m_scene->GetObjectByID(stats.targetID);
                if (target) {
                    UnitStats& tStats = target->unitStats;
                    int damage = stats.attack;
                    tStats.health -= damage;
                    if (tStats.health < 0) tStats.health = 0;

                    if (tStats.health <= 0) {
                        tStats.isAlive = false;
                        target->SetActive(false);
                        m_deadThisFrame.push_back(target->GetID());
                    }

                    stats.attackCooldown = 1.0f; // 1 attack per second
                }
            }
        }
    }

} // namespace gps
