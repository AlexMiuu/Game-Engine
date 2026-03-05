//
// Scene.cpp
// Implementarea clasei Scene
//

#include "Scene.hpp"
#include <algorithm>
#include <iostream>
#include "SceneObject.hpp"
namespace gps {

    Scene::Scene(const std::string& name)
        : m_name(name)
        , m_nextID(1)
    {
    }

    Scene::~Scene() {
        Clear();
    }

    // ===========================
    // OBJECT MANAGEMENT
    // ===========================

    SceneObject* Scene::CreateObject(const std::string& name) {
        int id = m_nextID++;

        auto obj = std::make_unique<SceneObject>(id, name);
        SceneObject* ptr = obj.get();

        // Salveazã în map pentru quick lookup
        m_objectsByID[id] = ptr;

        // Salveazã ownership
        m_objects.push_back(std::move(obj));

        return ptr;
    }

    void Scene::DestroyObject(int id) {
        auto it = m_objectsByID.find(id);
        if (it != m_objectsByID.end()) {
            DestroyObject(it->second);
        }
    }

    void Scene::DestroyObject(SceneObject* obj) {
        if (!obj) return;

        // ?terge din map
        m_objectsByID.erase(obj->GetID());

        // ?terge din vector
        m_objects.erase(
            std::remove_if(m_objects.begin(), m_objects.end(),
                [obj](const std::unique_ptr<SceneObject>& o) {
                    return o.get() == obj;
                }),
            m_objects.end()
        );
    }

    void Scene::Clear() {
        m_objects.clear();
        m_objectsByID.clear();
        m_nextID = 1;
    }

    // ===========================
    // OBJECT QUERIES
    // ===========================

    SceneObject* Scene::GetObjectByID(int id) {
        auto it = m_objectsByID.find(id);
        if (it != m_objectsByID.end()) {
            return it->second;
        }
        return nullptr;
    }

    SceneObject* Scene::GetObjectByName(const std::string& name) {
        for (auto& obj : m_objects) {
            if (obj->GetName() == name) {
                return obj.get();
            }
        }
        return nullptr;
    }

    std::vector<SceneObject*> Scene::GetObjectsByName(const std::string& name) {
        std::vector<SceneObject*> result;
        for (auto& obj : m_objects) {
            if (obj->GetName() == name) {
                result.push_back(obj.get());
            }
        }
        return result;
    }

    std::vector<SceneObject*> Scene::GetObjectsRaw() const {
        std::vector<SceneObject*> result;
        result.reserve(m_objects.size());
        for (const auto& obj : m_objects) {
            result.push_back(obj.get());
        }
        return result;
    }

    // ===========================
    // LIFECYCLE
    // ===========================

    void Scene::Update(float deltaTime) {
        // Update toate obiectele active
        for (auto& obj : m_objects) {
            if (obj && obj->IsActive()) {
                obj->Update(deltaTime);
            }
        }
    }

} // namespace gps