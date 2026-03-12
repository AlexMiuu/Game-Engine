//
// SceneManager.cpp
// Implementarea SceneManager
//

#include "SceneManager.hpp"
#include <iostream>
#include <cmath>
#include <limits>

namespace gps {

    SceneManager::SceneManager()
        : m_scene(nullptr)
        , m_tileManager(nullptr)
        , m_nextTroopID(100)  // �ncepe de la 100 pentru trupe
        , m_nextObjectID(1)   // �ncepe de la 1 pentru obiecte generale
        , m_spawnEnabled(true)
        , m_troopSpawnPos(100.0f, -60.0f, -90.0f)
        , m_troopCount(0)
    {
    }

    SceneManager::~SceneManager() {
        // Scene-ul e gestionat extern, nu �l ?tergem aici
    }

    // ===========================
    // INITIALIZATION
    // ===========================

    void SceneManager::Initialize(Scene* scene, TileManager* tileManager) {
        m_scene = scene;
        m_tileManager = tileManager;

        std::cout << "? SceneManager initialized" << std::endl;
    }

    void SceneManager::LoadModels() {
        // Aceast� metod� poate fi extins� pentru a �nc�rca modele
        // Pentru moment, presupunem c� modelele sunt �nc�rcate extern
        // ?i �nregistrate cu RegisterModel()

        std::cout << "?? Models loaded (external)" << std::endl;
    }

    void SceneManager::SetupScene() {
        if (!m_scene) {
            std::cerr << "? Scene is null! Cannot setup." << std::endl;
            return;
        }

        std::cout << "?? Setting up scene..." << std::endl;

        // Creeaz� componentele scenei
        CreateTerrain();
        CreateStaticObjects();
        CreateInitialTroops(5);

        std::cout << "? Scene setup complete! Objects in scene: "
            << m_scene->GetObjectCount() << std::endl;
    }

    // ===========================
    // SPAWN SYSTEM
    // ===========================

    SceneObject* SceneManager::SpawnTroop(const glm::vec3& position, const std::string& modelName) {
        if (!m_scene) {
            std::cerr << "? Cannot spawn troop: scene is null" << std::endl;
            return nullptr;
        }

        if (!m_spawnEnabled) {
            std::cout << "?? Spawn is disabled" << std::endl;
            return nullptr;
        }

        // G�se?te modelul
        Model3D* model = GetModel(modelName);
        if (!model) {
            std::cerr << "? Model '" << modelName << "' not found!" << std::endl;
            return nullptr;
        }

        // Creeaz� obiectul
        int troopID = GetNextTroopID();
        SceneObject* troop = m_scene->CreateObject("Troop_" + std::to_string(troopID));

        // Configureaz� transform
        troop->GetTransform().SetPosition(position);
        troop->GetTransform().SetScale(glm::vec3(1,1,1));

        // Seteaz� modelul
        troop->SetModel(model);

        // Calculeaz� bounding sphere
        ComputeAndSetBoundingSphere(troop, model);

        // Set collision radius
        troop->SetCollisionRadius(5.0f);

        // Incrementeaz� counter
        m_troopCount++;

        std::cout << "??? Spawned troop " << troopID << " at position ("
            << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;

        return troop;
    }

    std::vector<SceneObject*> SceneManager::SpawnTroopFormation(
        const glm::vec3& centerPosition,
        int count,
        float spacing,
        const std::string& modelName)
    {
        std::vector<SceneObject*> troops;

        if (count <= 0) return troops;

        // Calculeaz� grid pentru forma?ie
        int columns = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(count))));

        for (int i = 0; i < count; i++) {
            int row = i / columns;
            int col = i % columns;

            float offsetX = (col - columns / 2.0f) * spacing;
            float offsetZ = (row - count / columns / 2.0f) * spacing;

            glm::vec3 position = centerPosition + glm::vec3(offsetX, 0.0f, offsetZ);

            SceneObject* troop = SpawnTroop(position, modelName);
            if (troop) {
                troops.push_back(troop);
            }
        }

        std::cout << "??? Spawned formation of " << troops.size() << " troops" << std::endl;

        return troops;
    }

    SceneObject* SceneManager::SpawnObject(
        const std::string& name,
        const std::string& modelName,
        const glm::vec3& position,
        const glm::vec3& scale)
    {
        if (!m_scene) {
            std::cerr << "? Cannot spawn object: scene is null" << std::endl;
            return nullptr;
        }

        Model3D* model = GetModel(modelName);
        if (!model) {
            std::cerr << "? Model '" << modelName << "' not found!" << std::endl;
            return nullptr;
        }

        SceneObject* obj = m_scene->CreateObject(name);
        obj->GetTransform().SetPosition(position);
        obj->GetTransform().SetScale(scale);
        obj->SetModel(model);

        ComputeAndSetBoundingSphere(obj, model);

        std::cout << "?? Spawned object '" << name << "' (ID: " << obj->GetID() << ")" << std::endl;

        return obj;
    }

    // ===========================
    // SCENE SETUP COMPONENTS
    // ===========================

    void SceneManager::CreateTerrain() {
        std::cout << "??? Creating terrain..." << std::endl;

        Model3D* terrainModel = GetModel("terrain");
        if (!terrainModel) {
            std::cout << "?? Terrain model not found, skipping" << std::endl;
            return;
        }

        SceneObject* terrain = m_scene->CreateObject("Terrain");
        terrain->GetTransform().SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        terrain->SetModel(terrainModel);

        ComputeAndSetBoundingSphere(terrain, terrainModel);

        std::cout << "? Terrain created" << std::endl;
    }

    void SceneManager::CreateStaticObjects() {
        std::cout << "??? Creating static objects..." << std::endl;

        Model3D* objectsModel = GetModel("objects");
        if (!objectsModel) {
            std::cout << "?? Objects model not found, skipping" << std::endl;
            return;
        }

        SceneObject* objects = m_scene->CreateObject("StaticObjects");
        objects->GetTransform().SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        objects->SetModel(objectsModel);

        ComputeAndSetBoundingSphere(objects, objectsModel);

        std::cout << "? Static objects created" << std::endl;
    }

    void SceneManager::CreateInitialTroops(int count) {
        std::cout << "??? Creating initial troops (" << count << ")..." << std::endl;

        Model3D* orcModel = GetModel("pikeman");
        if (!orcModel) {
            std::cout << "?? Orc model not found, skipping troops" << std::endl;
            return;
        }

        // Spawn �n forma?ie la pozi?ia ini?ial�
        float spacing = 15.0f;
        int columns = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(count))));

        for (int i = 0; i < count; i++) {
            int row = i / columns;
            int col = i % columns;

            float offsetX = (col - columns / 2.0f) * spacing;
            float offsetZ = (row - count / columns / 2.0f) * spacing;

            glm::vec3 position = m_troopSpawnPos + glm::vec3(offsetX, 0.0f, offsetZ);

            SceneObject* troop = m_scene->CreateObject("Orc");
            troop->GetTransform().SetPosition(position);
            troop->SetModel(orcModel);

            ComputeAndSetBoundingSphere(troop, orcModel);

            // Set collision radius
            troop->SetCollisionRadius(5.0f);

            m_troopCount++;
        }

        std::cout << "? Created " << count << " initial troops" << std::endl;
    }


    // ===========================
    // MODEL MANAGEMENT
    // ===========================

    void SceneManager::RegisterModel(const std::string& name, Model3D* model) {
        if (!model) {
            std::cerr << "? Cannot register null model: " << name << std::endl;
            return;
        }

        m_models[name] = model;
        std::cout << "?? Registered model: " << name << std::endl;
    }

    Model3D* SceneManager::GetModel(const std::string& name) {
        auto it = m_models.find(name);
        if (it != m_models.end()) {
            return it->second;
        }
        return nullptr;
    }

    // ===========================
    // BOUNDING SPHERE HELPERS
    // ===========================

    void SceneManager::ComputeAndSetBoundingSphere(SceneObject* obj, Model3D* model) {
        if (!obj || !model) return;

        glm::vec3 center;
        float radius;

        ComputeLocalBoundingSphere(model, center, radius);

        obj->SetLocalBounds(center, radius);
        obj->UpdateWorldBounds();
    }

    void SceneManager::ComputeLocalBoundingSphere(Model3D* model, glm::vec3& outCenter, float& outRadius) {
        glm::vec3 minPos(std::numeric_limits<float>::max());
        glm::vec3 maxPos(std::numeric_limits<float>::lowest());

        for (const auto& mesh : model->getMeshes()) {
            for (const auto& vertex : mesh.vertices) {
                minPos = glm::min(minPos, vertex.Position);
                maxPos = glm::max(maxPos, vertex.Position);
            }
        }

        outCenter = 0.5f * (minPos + maxPos);
        outRadius = glm::length(maxPos - minPos) * 0.5f;
    }

} // namespace gps