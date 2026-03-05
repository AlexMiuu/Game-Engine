//
// Scene.hpp
// Container pentru toate obiectele din scenã
//

#ifndef SCENE_HPP
#define SCENE_HPP

#include "SceneObject.hpp"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

namespace gps {

    /**
     * @brief Container pentru toate obiectele din scenã
     *
     * Înlocuie?te: std::vector<SceneObject> g_sceneObjects din main.cpp
     *
     * Exemplu de utilizare:
     * @code
     * Scene scene("MainScene");
     *
     * // Creeazã obiecte
     * SceneObject* terrain = scene.CreateObject("Terrain");
     * SceneObject* orc = scene.CreateObject("Orc");
     *
     * // Query obiecte
     * SceneObject* obj = scene.GetObjectByID(1);
     * std::vector<SceneObject*> troops = scene.GetObjectsByName("Troop");
     *
     * // Update toate
     * scene.Update(deltaTime);
     * @endcode
     */
    class Scene {
    public:
        Scene(const std::string& name = "Scene");
        ~Scene();

        // ===========================
        // OBJECT MANAGEMENT
        // ===========================

        /**
         * @brief Creeazã un obiect nou în scenã
         * @param name Numele obiectului (op?ional)
         * @return Pointer la obiectul creat
         */
        SceneObject* CreateObject(const std::string& name = "GameObject");

        /**
         * @brief ?terge un obiect din scenã
         */
        void DestroyObject(int id);
        void DestroyObject(SceneObject* obj);

        /**
         * @brief ?terge toate obiectele
         */
        void Clear();

        // ===========================
        // OBJECT QUERIES
        // ===========================

        /**
         * @brief Gãse?te obiect dupã ID
         * @return Pointer la obiect sau nullptr
         */
        SceneObject* GetObjectByID(int id);

        /**
         * @brief Gãse?te primul obiect cu numele specificat
         */
        SceneObject* GetObjectByName(const std::string& name);

        /**
         * @brief Gãse?te toate obiectele cu numele specificat
         */
        std::vector<SceneObject*> GetObjectsByName(const std::string& name);

        /**
         * @brief Ob?ine toate obiectele din scenã
         */
        const std::vector<std::unique_ptr<SceneObject>>& GetObjects() const {
            return m_objects;
        }

        /**
         * @brief Ob?ine numãrul de obiecte
         */
        size_t GetObjectCount() const { return m_objects.size(); }

        // ===========================
        // LIFECYCLE
        // ===========================

        /**
         * @brief Update toate obiectele active
         */
        void Update(float deltaTime);

        // ===========================
        // SCENE INFO
        // ===========================

        const std::string& GetName() const { return m_name; }
        void SetName(const std::string& name) { m_name = name; }

        // ===========================
        // COMPATIBILITATE cu codul vechi
        // ===========================

        /**
         * @brief Converte?te la vector simplu pentru compatibilitate
         * ATEN?IE: Returneazã pointeri RAW - nu ?terge manual!
         */
        std::vector<SceneObject*> GetObjectsRaw() const;

    private:
        std::string m_name;
        std::vector<std::unique_ptr<SceneObject>> m_objects;
        int m_nextID;

        // Pentru quick lookup
        std::unordered_map<int, SceneObject*> m_objectsByID;
    };

} // namespace gps

#endif // SCENE_HPP