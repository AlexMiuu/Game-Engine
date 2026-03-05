//
// SceneManager.hpp
// Manager pentru ini?ializarea ?i spawn-ul obiectelor în scenã
//

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

    /**
     * @brief Manager pentru crearea ?i gestionarea obiectelor în scenã
     *
     * Responsabilitã?i:
     * - Ini?ializeazã scena cu toate obiectele (teren, clãdiri, unitã?i)
     * - Spawn-uie?te unitã?i noi în timpul jocului
     * - Gestioneazã ID-urile obiectelor
     * - Configureazã properties ale obiectelor (bounding spheres, etc)
     *
     * Exemplu de utilizare:
     * @code
     * SceneManager sceneManager;
     * sceneManager.Initialize(scene, tileManager);
     * sceneManager.LoadModels();
     * sceneManager.SetupScene();
     *
     * // Spawn unitã?i în timpul jocului
     * SceneObject* troop = sceneManager.SpawnTroop(glm::vec3(100, -60, -90));
     * @endcode
     */
    class SceneManager {
    public:
        SceneManager();
        ~SceneManager();

        // ===========================
        // INITIALIZATION
        // ===========================

        /**
         * @brief Ini?ializeazã scene manager-ul cu scene ?i tile manager
         * @param scene Pointer la scenã (va fi gestionatã de SceneManager)
         * @param tileManager Pointer la tile manager (pentru teren)
         */
        void Initialize(Scene* scene, TileManager* tileManager = nullptr);

        /**
         * @brief Încarcã toate modelele necesare
         * Aceastã metodã trebuie apelatã DUPÃ ini?ializarea OpenGL
         */
        void LoadModels();

        /**
         * @brief Configureazã scena ini?ialã cu toate obiectele
         * Creeazã terenul, obiectele statice, unitã?i ini?iale, etc.
         */
        void SetupScene();

        // ===========================
        // SPAWN SYSTEM
        // ===========================

        /**
         * @brief Spawnezã o trupã la pozi?ia specificatã
         * @param position Pozi?ia în world space
         * @param modelName Numele modelului (default: "orc")
         * @return Pointer la obiectul creat sau nullptr dacã a e?uat
         */
        SceneObject* SpawnTroop(const glm::vec3& position, const std::string& modelName = "orc");

        /**
         * @brief Spawnezã mai multe trupe într-o forma?ie
         * @param centerPosition Centrul forma?iei
         * @param count Numãrul de trupe
         * @param spacing Spa?iul între trupe
         * @param modelName Numele modelului
         * @return Vector cu pointeri la trupele create
         */
        std::vector<SceneObject*> SpawnTroopFormation(
            const glm::vec3& centerPosition,
            int count,
            float spacing = 15.0f,
            const std::string& modelName = "orc"
        );

        /**
         * @brief Spawnezã un obiect generic
         * @param name Numele obiectului
         * @param modelName Numele modelului
         * @param position Pozi?ia
         * @param scale Scale-ul
         * @return Pointer la obiectul creat
         */
        SceneObject* SpawnObject(
            const std::string& name,
            const std::string& modelName,
            const glm::vec3& position,
            const glm::vec3& scale = glm::vec3(1.0f)
        );

        // ===========================
        // SCENE SETUP COMPONENTS
        // ===========================

        /**
         * @brief Creeazã terenul principal
         */
        void CreateTerrain();

        /**
         * @brief Creeazã obiectele statice (ziduri, decora?iuni)
         */
        void CreateStaticObjects();

        /**
         * @brief Creeazã unitã?ile ini?iale
         * @param count Numãrul de unitã?i de creat
         */
        void CreateInitialTroops(int count = 5);

        /**
         * @brief Creeazã obiectele animate (dragoni, etc)
         */
        void CreateAnimatedObjects();

        // ===========================
        // MODEL MANAGEMENT
        // ===========================

        /**
         * @brief Înregistreazã un model pentru folosire ulterioarã
         * @param name Numele modelului (pentru referin?ã)
         * @param model Pointer la model
         */
        void RegisterModel(const std::string& name, Model3D* model);

        /**
         * @brief Ob?ine un model dupã nume
         * @return Pointer la model sau nullptr dacã nu existã
         */
        Model3D* GetModel(const std::string& name);

        // ===========================
        // QUERIES
        // ===========================

        /**
         * @brief Ob?ine scena gestionatã
         */
        Scene* GetScene() { return m_scene; }

        /**
         * @brief Ob?ine urmãtorul ID disponibil pentru trupe
         */
        int GetNextTroopID() { return m_nextTroopID++; }

        /**
         * @brief Ob?ine numãrul total de trupe spawneate
         */
        int GetTroopCount() const { return m_troopCount; }

        /**
         * @brief Verificã dacã spawn-ul e activat
         */
        bool IsSpawnEnabled() const { return m_spawnEnabled; }
        void SetSpawnEnabled(bool enabled) { m_spawnEnabled = enabled; }

        // ===========================
        // CONFIGURATION
        // ===========================

        /**
         * @brief Seteazã pozi?ia de spawn pentru trupe
         */
        void SetTroopSpawnPosition(const glm::vec3& pos) { m_troopSpawnPos = pos; }

        /**
         * @brief Ob?ine pozi?ia de spawn pentru trupe
         */
        glm::vec3 GetTroopSpawnPosition() const { return m_troopSpawnPos; }

        // ===========================
        // BOUNDING SPHERE HELPERS
        // ===========================

        /**
         * @brief Calculeazã ?i seteazã bounding sphere pentru un obiect
         * @param obj Obiectul pentru care se calculeazã
         * @param model Modelul 3D al obiectului
         */
        void ComputeAndSetBoundingSphere(SceneObject* obj, Model3D* model);

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

} // namespace gps

#endif // SCENE_MANAGER_HPP