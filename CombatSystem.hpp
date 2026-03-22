//
// CombatSystem.hpp
// Per-frame combat resolution: target finding, damage, death collection
//

#ifndef COMBAT_SYSTEM_HPP
#define COMBAT_SYSTEM_HPP

#include "Scene.hpp"
#include "SceneObject.hpp"
#include <vector>

namespace gps {

    class CombatSystem {
    public:
        CombatSystem();
        ~CombatSystem();

        CombatSystem(const CombatSystem&) = delete;
        CombatSystem& operator=(const CombatSystem&) = delete;

        void Initialize(Scene* scene);

        // Call once per frame — resolves all combat, marks dead units
        void Update(float deltaTime);

        // IDs of units that died this frame — remove from scene after Update()
        const std::vector<int>& GetDeadIDs() const { return m_deadThisFrame; }
        void ClearDeadIDs() { m_deadThisFrame.clear(); }

        void SetEnabled(bool enabled) { m_enabled = enabled; }
        bool IsEnabled() const { return m_enabled; }

    private:
        Scene* m_scene  = nullptr;
        bool   m_enabled = true;
        std::vector<int> m_deadThisFrame;

        // Returns ID of nearest enemy in attack range, or -1
        int FindNearestEnemy(SceneObject* attacker) const;
    };

} // namespace gps

#endif // COMBAT_SYSTEM_HPP
