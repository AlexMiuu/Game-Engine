//
// SceneManager.hpp
// Manager pentru ini?ializarea ?i spawn-ul obiectelor �n scen�
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
     * @brief Manager pentru crearea ?i gestionarea obiectelor �n scen�
     *
     * Responsabilit�?i:
     * - Ini?ializeaz� scena cu toate obiectele (teren, cl�diri, unit�?i)
     * - Spawn-uie?te unit�?i noi �n timpul jocului
     * - Gestioneaz� ID-urile obiectelor
     * - Configureaz� properties ale obiectelor (bounding spheres, etc)
     *
     * Exemplu de utilizare:
     * @code
     * SceneManager sceneManager;
     * sceneManager.Initialize(scene, tileManager);
     * sceneManager.LoadModels();
     * sceneManager.SetupScene();
     *
     * // Spawn unit�?i �n timpul jocului
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
         * @brief Ini?ializeaz� scene manager-ul cu scene ?i tile manager
         * @param scene Pointer la scen� (va fi gestionat� de SceneManager)
         * @param tileManager Pointer la tile manager (pentru teren)
         */
        void Initialize(Scene* scene, TileManager* tileManager = nullptr);

        /**
         * @brief �ncarc� toate modelele necesare
         * Aceast� metod� trebuie apelat� DUP� ini?ializarea OpenGL
         */
        void LoadModels();

        /**
         * @brief Configureaz� scena ini?ial� cu toate obiectele
         * Creeaz� terenul, obiectele statice, unit�?i ini?iale, etc.
         */
        void SetupScene();

        // ===========================
        // SPAWN SYSTEM
        // ===========================

        /**
         * @brief Spawnez� o trup� la pozi?ia specificat�
         * @param position Pozi?ia �n world space
         * @param modelName Numele modelului (default: "orc")
         * @return Pointer la obiectul creat sau nullptr dac� a e?uat
         */
        SceneObject* SpawnTroop(const glm::vec3& position, const std::string& modelName = "orc");

        /**
         * @brief Spawnez� mai multe trupe �ntr-o forma?ie
         * @param centerPosition Centrul forma?iei
         * @param count Num�rul de trupe
         * @param spacing Spa?iul �ntre trupe
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
         * @brief Spawnez� un obiect generic
         * @param name Numele obiectului
         * @param modelName Numele modelului
         * @param position Pozi?ia
         * @param scale Scale-ul
         * @return Pointer la obiectul creat
         */
        SceneObject* SpawnObject(
            const std::string& name,
            const std::string& tag,
            const std::string& modelName,
            const glm::vec3& position,
            const glm::vec3& scale = glm::vec3(1.0f)
        );

        // ===========================
        // SCENE SETUP COMPONENTS
        // ===========================

        /**
         * @brief Creeaz� terenul principal
         */
        void CreateTerrain();

        /**
         * @brief Creeaz� obiectele statice (ziduri, decora?iuni)
         */
        void CreateStaticObjects();

        /**
         * @brief Creeaz� unit�?ile ini?iale
         * @param count Num�rul de unit�?i de creat
         */
        void CreateInitialTroops(int count = 5);

        /**
         * @brief Creeaz� obiectele animate (dragoni, etc)
         */
        void CreateAnimatedObjects();

        // ===========================
        // MODEL MANAGEMENT
        // ===========================

        /**
         * @brief �nregistreaz� un model pentru folosire ulterioar�
         * @param name Numele modelului (pentru referin?�)
         * @param model Pointer la model
         */
        void RegisterModel(const std::string& name, Model3D* model);

        /**
         * @brief Ob?ine un model dup� nume
         * @return Pointer la model sau nullptr dac� nu exist�
         */
        Model3D* GetModel(const std::string& name);

        // ===========================
        // QUERIES
        // ===========================

        /**
         * @brief Ob?ine scena gestionat�
         */
        Scene* GetScene() { return m_scene; }

        /**
         * @brief Ob?ine urm�torul ID disponibil pentru trupe
         */
        int GetNextTroopID() { return m_nextTroopID++; }

        /**
         * @brief Ob?ine num�rul total de trupe spawneate
         */
        int GetTroopCount() const { return m_troopCount; }

        /**
         * @brief Verific� dac� spawn-ul e activat
         */
        bool IsSpawnEnabled() const { return m_spawnEnabled; }
        void SetSpawnEnabled(bool enabled) { m_spawnEnabled = enabled; }

        // ===========================
        // CONFIGURATION
        // ===========================

        /**
         * @brief Seteaz� pozi?ia de spawn pentru trupe
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
         * @brief Calculeaz� ?i seteaz� bounding sphere pentru un obiect
         * @param obj Obiectul pentru care se calculeaz�
         * @param model Modelul 3D al obiectului
         */
        void ComputeAndSetBoundingSphere(SceneObject* obj, Model3D* model);

        /**
         * @brief Initializes unit stats based on the object's name tag
         * Call this after spawning any object.
         */
        UnitStats InitializeUnitsStats(SceneObject* object);

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