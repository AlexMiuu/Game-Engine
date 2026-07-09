//
// Scene.hpp
// Container pentru toate obiectele din scen�
//

#ifndef SCENE_HPP
#define SCENE_HPP

#include "SceneObject.hpp"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

namespace gps {

    class Scene {
    public:
        Scene(const std::string& name = "Scene");
        ~Scene();

        SceneObject* CreateObject(const std::string& name = "GameObject");

        void DestroyObject(int id);
        void DestroyObject(SceneObject* obj);

        void Clear();

        SceneObject* GetObjectByID(int id);
        SceneObject* GetObjectByName(const std::string& name);
        std::vector<SceneObject*> GetObjectsByName(const std::string& name);
        const std::vector<std::unique_ptr<SceneObject>>& GetObjects() const {
            return m_objects;
        }
        size_t GetObjectCount() const { return m_objects.size(); }


        void Update(float deltaTime);

        const std::string& GetName() const { return m_name; }
        void SetName(const std::string& name) { m_name = name; }

        std::vector<SceneObject*> GetObjectsRaw() const;

    private:
        std::string m_name;
        std::vector<std::unique_ptr<SceneObject>> m_objects;
        int m_nextID;

        // Pentru quick lookup
        std::unordered_map<int, SceneObject*> m_objectsByID;
    };

}

#endif