#ifndef SCENE_MANAGER_HPP
#define SCENE_MANAGER_HPP

#include "Scene.hpp"
#include "SceneObject.hpp"
#include "Model3D.hpp"
#include "TileManager.hpp"
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>
#include <string>

namespace gps {

    UnitStats::PricePoints GetPropPrice(const std::string& tag);

    class SceneManager {
    public:
        SceneManager();
        ~SceneManager();

        void Initialize(Scene* scene, TileManager* tileManager = nullptr);

        void LoadModels();
        void SetupScene();


        SceneObject* SpawnTroop(const glm::vec3& position, const std::string& modelName = "orc");
        std::vector<SceneObject*> SpawnTroopFormation(
            const glm::vec3& centerPosition,
            int count,
            float spacing = 15.0f,
            const std::string& modelName = "orc"
        );

        SceneObject* SpawnObject(
            const std::string& name,
            const std::string& tag,
            const std::string& modelName,
            const glm::vec3& position,
            const glm::vec3& scale = glm::vec3(1.0f)
        );

        void CreateTerrain();
        void CreateStaticObjects();
        void CreateInitialTroops(int count = 5);
        void CreateAnimatedObjects();

        void RegisterModel(const std::string& name, Model3D* model);
        Model3D* GetModel(const std::string& name);

        Scene* GetScene() { return m_scene; }
        int GetNextTroopID() { return m_nextTroopID++; }
        int GetTroopCount() const { return m_troopCount; }

        bool IsSpawnEnabled() const { return m_spawnEnabled; }
        void SetSpawnEnabled(bool enabled) { m_spawnEnabled = enabled; }

        void SetTroopSpawnPosition(const glm::vec3& pos) { m_troopSpawnPos = pos; }
        glm::vec3 GetTroopSpawnPosition() const { return m_troopSpawnPos; }


        void ComputeAndSetBoundingSphere(SceneObject* obj, Model3D* model);
        void SetPropPlacement(const std::string& modelName, const std::string& tag, const glm::vec3& scale, const std::string& label, int faction = 1);
        void CancelPropPlacement();

        UnitStats InitializeUnitsStats(SceneObject* object);

        void CreateRandomObstacles(int count);
        

    public:

        std::string m_propPlacementModelName;
        std::string m_propPlacementTag;
        glm::vec3 m_propPlacementScale;
        std::string m_propPlacementLabel;
        bool m_propPlacementMode=false;
        int  m_propPlacementFaction = 1; // 1 = friendly, 2 = enemy

        bool m_bombardmentTargeting = false;

        // Patrol targeting: GUI/hotkey enters this mode with a snapshot of the
        // current selection; the next two left-clicks define points A and B.
        bool             m_patrolTargeting = false;
        int              m_patrolClickPhase = 0; // 0 = waiting for A, 1 = waiting for B
        glm::vec3        m_patrolPointA = glm::vec3(0.0f);
        std::vector<int> m_patrolUnitIDs;

    private:
        // Scene reference
        Scene* m_scene;
        TileManager* m_tileManager;

        // Model registry (nu owner - doar referin?e)
        std::unordered_map<std::string, Model3D*> m_models;

        // ID generators
        int m_nextTroopID;
        int m_nextObjectID;

        // Spawn settings
        bool m_spawnEnabled;
        glm::vec3 m_troopSpawnPos;
        int m_troopCount;

        // Helper methods
        void ComputeLocalBoundingSphere(Model3D* model, glm::vec3& outCenter, float& outRadius);
    };

}

#endif