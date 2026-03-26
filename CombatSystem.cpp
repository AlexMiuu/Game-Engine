//
// CombatSystem.cpp
//

#include "CombatSystem.hpp"
#include "SceneManager.hpp"
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <iostream>

namespace gps {

    CombatSystem::CombatSystem() = default;
    CombatSystem::~CombatSystem() = default;

    void CombatSystem::Initialize(Scene* scene, SceneManager* sceneManager) {
        m_scene = scene;
        m_sceneManager = sceneManager;
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
            if (candidate->projectileData.isProjectile) continue;

            const UnitStats& cStats = candidate->unitStats;
            if (!cStats.isAlive) continue;

            // Skip same-faction units (faction 0 = neutral, always targetable)
            if (atkStats.faction != 0 && cStats.faction != 0 && cStats.faction == atkStats.faction) continue;

            float distance = glm::distance(attacker->GetWorldCenter(), candidate->GetWorldCenter());

            if (distance > atkStats.attackRange) {
                continue; // out of range, skip
            }

            // In range — pick nearest
            float distSq = distance * distance;
            if (distSq < bestDistSq) {
                bestDistSq = distSq;
                bestID = candidate->GetID();
            }
        }

        return bestID;
    }


    void CombatSystem::SpawnCannonBall(SceneObject* origin, SceneObject* target, double fuseTime) {
        if (!m_sceneManager) return;

        SceneObject* cannonBall = m_sceneManager->SpawnObject("orc", "orc", "orc",
            origin->GetTransform().GetPosition(), glm::vec3(0.5f, 0.5f, 0.5f));
        if (!cannonBall) return;

        cannonBall->projectileData.isProjectile = true;
        cannonBall->projectileData.ownerID      = origin->GetID();
        cannonBall->projectileData.targetID     = target->GetID();
        cannonBall->projectileData.damage       = (float)origin->unitStats.attack;

        cannonBall->movement.isMoving      = true;
        cannonBall->movement.moveStartPos  = origin->GetWorldCenter();
        cannonBall->movement.moveEndPos    = target->GetWorldCenter();
        cannonBall->movement.moveDirection = glm::normalize(cannonBall->movement.moveEndPos - cannonBall->movement.moveStartPos);
        cannonBall->movement.moveStartTime = (float)glfwGetTime();
        cannonBall->movement.moveDuration  = static_cast<float>(fuseTime);
    }

    void CombatSystem::Update(float deltaTime) {
        if (!m_scene || !m_enabled) return;

        // Snapshot to avoid iterator invalidation when SpawnCannonBall adds objects
        std::vector<SceneObject*> snapshot;
        snapshot.reserve(m_scene->GetObjects().size());
        for (const auto& obj : m_scene->GetObjects())
            snapshot.push_back(obj.get());

        struct PendingShot { SceneObject* origin; SceneObject* target; double travelTime; };
        std::vector<PendingShot> pendingShots;

        for (SceneObject* attacker : snapshot) {
            if (!attacker->IsActive()) continue;
            if (attacker->projectileData.isProjectile) continue;

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
                    float dist = glm::distance(attacker->GetWorldCenter(), target->GetWorldCenter());
                    if (dist > stats.attackRange)
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
                    bool isShip = (attacker->GetTag() == "ship");
                    if (isShip) {
                        float dist = glm::distance(attacker->GetWorldCenter(), target->GetWorldCenter());
                        pendingShots.push_back({ attacker, target, dist / 80.0 });
                    } else {
                        UnitStats& tStats = target->unitStats;
                        tStats.health -= stats.attack;
                        if (tStats.health < 0) tStats.health = 0;
                        if (tStats.health <= 0) {
                            tStats.isAlive = false;
                            target->SetActive(false);
                            m_deadThisFrame.push_back(target->GetID());
                        }
                    }
                    stats.attackCooldown = 1.0f;
                }
            }
        }

        // Spawn cannonballs after the loop to avoid iterator invalidation
        for (auto& shot : pendingShots)
            SpawnCannonBall(shot.origin, shot.target, shot.travelTime);
    }

} // namespace gps
