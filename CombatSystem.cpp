//
// CombatSystem.cpp
//

#include "CombatSystem.hpp"
#include "SceneManager.hpp"
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <cmath>
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
            if (candidate->orbitData.isOrbiting) continue;

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

        SceneObject* cannonBall = m_sceneManager->SpawnObject("projectile", "projectile", "projectile",
            origin->GetTransform().GetPosition(), glm::vec3(7.5f, 7.5f, 7.5f));
        if (!cannonBall) return;

        cannonBall->projectileData.isProjectile = true;
        cannonBall->projectileData.ownerID      = origin->GetID();
        cannonBall->projectileData.targetID     = target->GetID();
        float dmg = (float)origin->unitStats.attack;
        if (origin->unitStats.stance == CombatStance::Defensive) dmg *= 1.3f;
        cannonBall->projectileData.damage       = dmg;
        cannonBall->projectileData.splashRadius =
            (origin->unitStats.attackMode == AttackMode::ProjectileSplash)
            ? origin->unitStats.splashRadius : 0.0f;

        // CIWS rounds render red.
        if (origin->GetTag() == "turret")
            cannonBall->projectileData.tint = glm::vec3(1.0f, 0.15f, 0.15f);

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

            // Find new target if none.
            // Defensive stance holds ground: never auto-acquires; only fights back
            // at whoever assigned itself as targetID via retaliation.
            if (stats.targetID == -1 && stats.stance != CombatStance::Defensive)
                stats.targetID = FindNearestEnemy(attacker);

            // Face current target — for stationary combat units (turrets) that don't
            // otherwise drive their own rotation. Skip orbiting units (aircraft already
            // set their yaw from orbit angle) and movable units (they yaw from movement).
            if (stats.targetID != -1 && !stats.isMovable && !attacker->orbitData.isOrbiting) {
                SceneObject* target = m_scene->GetObjectByID(stats.targetID);
                if (target) {
                    glm::vec3 dir = target->GetWorldCenter() - attacker->GetWorldCenter();
                    if (glm::length(dir) > 0.0001f) {
                        float yaw = glm::degrees(std::atan2(dir.x, dir.z));
                        glm::vec3 r = attacker->GetTransform().GetRotation();
                        attacker->GetTransform().SetRotation(glm::vec3(r.x, yaw, r.z));
                    }
                }
            }

            // Attack if ready and target exists
            if (stats.targetID != -1 && stats.attackCooldown <= 0.0f) {
                SceneObject* target = m_scene->GetObjectByID(stats.targetID);
                if (target) {
                    switch (stats.attackMode) {
                    case AttackMode::Projectile:
                    case AttackMode::ProjectileSplash: {
                        float dist = glm::distance(attacker->GetWorldCenter(), target->GetWorldCenter());
                        pendingShots.push_back({ attacker, target, dist / 80.0 });
                        break;
                    }
                    case AttackMode::Melee:
                    default: {
                        UnitStats& tStats = target->unitStats;
                        int dmg = stats.attack;
                        if (stats.stance == CombatStance::Defensive) dmg = (int)(dmg * 1.3f);
                        tStats.health -= dmg;
                        if (tStats.health < 0) tStats.health = 0;
                        if (tStats.health <= 0) {
                            tStats.isAlive = false;
                            target->SetActive(false);
                            m_deadThisFrame.push_back(target->GetID());
                        } else if (tStats.targetID == -1) {
                            // Retaliation: assign attacker so Defensive units fight back
                            tStats.targetID = attacker->GetID();
                        }
                        break;
                    }
                    }
                    float baseCD = stats.baseAttackCooldown;
                    stats.attackCooldown = (stats.stance == CombatStance::Aggressive) ? baseCD * 0.7f : baseCD;
                }
            }
        }

        // Spawn cannonballs after the loop to avoid iterator invalidation
        for (auto& shot : pendingShots)
            SpawnCannonBall(shot.origin, shot.target, shot.travelTime);
    }

} // namespace gps
