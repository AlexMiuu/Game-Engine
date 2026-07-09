#include "SceneManager.hpp"
#include <iostream>
#include <cmath>
#include <limits>
#include <algorithm>
#include <random>

namespace gps {

    SceneManager::SceneManager()
        : m_scene(nullptr)
        , m_tileManager(nullptr)
        , m_nextTroopID(100)
        , m_nextObjectID(1)
        , m_spawnEnabled(true)
        , m_troopSpawnPos(100.0f, -60.0f, -90.0f)
        , m_troopCount(0)
		, m_propPlacementMode(false)
		, m_propPlacementModelName("")
		, m_propPlacementTag("")
		, m_propPlacementScale(glm::vec3(1.0f))
		, m_propPlacementLabel("")

    {
    }

    SceneManager::~SceneManager() {
    }

    void SceneManager::Initialize(Scene* scene, TileManager* tileManager) {
        m_scene = scene;
        m_tileManager = tileManager;

        std::cout << "SceneManager initialized" << std::endl;
    }

    void SceneManager::LoadModels() {
        std::cout << "Models loaded (external)" << std::endl;
    }

    void SceneManager::SetupScene() {
        if (!m_scene) {
            std::cerr << "SceneManager: Scene is null! Cannot setup." << std::endl;
            return;
        }

        std::cout << "SceneManager: Setting up scene..." << std::endl;        

        std::cout << "SceneManager: Scene setup complete! Objects in scene: "
            << m_scene->GetObjectCount() << std::endl;
    }


    SceneObject* SceneManager::SpawnTroop(const glm::vec3& position, const std::string& modelName) {
        if (!m_scene) {
            std::cerr << "SceneManager: Cannot spawn troop: scene is null" << std::endl;
            return nullptr;
        }

        if (!m_spawnEnabled) {
            std::cout << "SceneManager: Spawn is disabled" << std::endl;
            return nullptr;
        }

        Model3D* model = GetModel(modelName);
        if (!model) {
            std::cerr << "Model '" << modelName << "' not found!" << std::endl;
            return nullptr;
        }

        int troopID = GetNextTroopID();
        SceneObject* troop = m_scene->CreateObject("Troop_" + std::to_string(troopID));

        troop->GetTransform().SetPosition(position);
        troop->GetTransform().SetScale(glm::vec3(1,1,1));

        troop->SetModel(model);

        ComputeAndSetBoundingSphere(troop, model);

        UnitStats objectStats = InitializeUnitsStats(troop);
        troop->unitStats = objectStats;

        // Set collision radius
        troop->SetCollisionRadius(25.0f);
        m_troopCount++;

        std::cout << "SceneManager: Spawned troop " << troopID << " at position ("
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

        std::cout << "SceneManager: Spawned formation of " << troops.size() << " troops" << std::endl;

        return troops;
    }

    void SceneManager::SetPropPlacement(const std::string& modelName,const std::string& tag, const glm::vec3& scale, const std::string& label, int faction){
        m_propPlacementModelName = modelName;
        m_propPlacementTag = tag;
        m_propPlacementScale = scale;
        m_propPlacementLabel = label;
        m_propPlacementMode = true;
        m_propPlacementFaction = faction;
        std::cout << "Prop placement mode enabled for model '" << modelName
                  << "' (faction " << faction << ")" << std::endl;

    }
    SceneObject* SceneManager::SpawnObject(
        const std::string& name,
        const std::string& tag,
        const std::string& modelName,
        const glm::vec3& position,
        const glm::vec3& scale)
    {
        if (!m_scene) {
            std::cerr << "Cannot spawn object: scene is null" << std::endl;
            return nullptr;
        }

        Model3D* model = GetModel(modelName);
        if (!model) {
            std::cerr << "Model '" << modelName << "' not found!" << std::endl;
            return nullptr;
        }

        SceneObject* obj = m_scene->CreateObject(name);
        obj->GetTransform().SetPosition(position);
        obj->GetTransform().SetScale(scale);
        obj->SetModel(model);
        obj->SetTag(tag);


        ComputeAndSetBoundingSphere(obj, model);
        
        UnitStats objectStats = InitializeUnitsStats(obj);
        obj->unitStats = objectStats;

        obj->SetCollisionRadius(25.0f);


        std::cout << "SceneManager: Spawned object '" << name << "' (ID: " << obj->GetID() << ")" << std::endl;

        return obj;
    }

    void SceneManager::CreateTerrain() {
        std::cout << "SceneManager: Creating terrain..." << std::endl;

        Model3D* terrainModel = GetModel("terrain");
        if (!terrainModel) {
            std::cout << "SceneManager: Terrain model not found, skipping" << std::endl;
            return;
        }

        SceneObject* terrain = m_scene->CreateObject("Terrain");
        terrain->GetTransform().SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        terrain->SetModel(terrainModel);

        ComputeAndSetBoundingSphere(terrain, terrainModel);

        std::cout << "SceneManager: Terrain created" << std::endl;
    }

    void SceneManager::CreateStaticObjects() {
        std::cout << "SceneManager: Creating static objects..." << std::endl;

        Model3D* objectsModel = GetModel("objects");
        if (!objectsModel) {
            std::cout << "SceneManager: Objects model not found, skipping" << std::endl;
            return;
        }

        SceneObject* objects = m_scene->CreateObject("StaticObjects");
        objects->GetTransform().SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        objects->SetModel(objectsModel);

        ComputeAndSetBoundingSphere(objects, objectsModel);

        std::cout << "SceneManager: Static objects created" << std::endl;
    }

    void SceneManager::CreateInitialTroops(int count) {
        std::cout << "SceneManager: Creating initial troops (" << count << ")..." << std::endl;

        Model3D* orcModel = GetModel("ship");
        if (!orcModel) {
            std::cout << "SceneManager: Orc model not found, skipping troops" << std::endl;
            return;
        }

        float spacing = 15.0f;
        int columns = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(count))));

        for (int i = 0; i < count; i++) {
            int row = i / columns;
            int col = i % columns;

            float offsetX = (col - columns / 2.0f) * spacing;
            float offsetZ = (row - count / columns / 2.0f) * spacing;

            glm::vec3 position = m_troopSpawnPos + glm::vec3(offsetX, 0.0f, offsetZ);

            SceneObject* troop = m_scene->CreateObject("Ship");
            troop->GetTransform().SetPosition(position);
            troop->SetModel(orcModel);
            troop->SetTag("ship");

            ComputeAndSetBoundingSphere(troop, orcModel);

            UnitStats objectStats = InitializeUnitsStats(troop);
            troop->unitStats = objectStats;

            // Set collision radius
            troop->SetCollisionRadius(5.0f);

            m_troopCount++;
        }

        std::cout << "SceneManager: Created " << count << " initial troops" << std::endl;
    }

    void SceneManager::CancelPropPlacement() {
        if (!m_propPlacementMode) return;
        m_propPlacementMode = false;
        std::cout << "SceneManager: Placement cancelled" << std::endl;
    }

    // ===========================
    // MODEL MANAGEMENT
    // ===========================

    void SceneManager::RegisterModel(const std::string& name, Model3D* model) {
        if (!model) {
            std::cerr << "SceneManager: Cannot register null model: " << name << std::endl;
            return;
        }

        m_models[name] = model;
        std::cout << "SceneManager: Registered model: " << name << std::endl;
    }

    Model3D* SceneManager::GetModel(const std::string& name) {
        auto it = m_models.find(name);
        if (it != m_models.end()) {
            return it->second;
        }
        return nullptr;
    }

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

    void SceneManager::CreateRandomObstacles(int count) {

        Model3D* islandT = GetModel("islandT");

        std::vector<Tile*> allTiles = m_tileManager->GetAllTiles();


        std::mt19937 rng(std::random_device{}());
        std::shuffle(allTiles.begin(), allTiles.end(), rng);

        float tileSize = m_tileManager->GetTileSize();
        float islandY = static_cast<float>(m_tileManager->GetTileHeight());
        float minDist = tileSize * 5.5f;   // half a tile apart minimum

        std::uniform_real_distribution<float> offsetDist(-tileSize * 0.25f, tileSize * 0.25f);
        std::vector<glm::vec3> placedPositions;
        int spawned = 0;

        for (Tile* tile : allTiles) 
        {
            if (spawned >= count) break;

            glm::vec3 pos = tile->worldPos;
            pos.y = islandY;
            pos.x += offsetDist(rng);
            pos.z += offsetDist(rng);

            // Minimum distance check against already-placed islands
            bool tooClose = false;
            for (const auto& placed : placedPositions) {
                if (glm::length(glm::vec2(pos.x - placed.x, pos.z - placed.z)) < minDist) {
                    tooClose = true;
                    break;
                }
            }
            if (tooClose) continue;

            std::string name = "Island_" + std::to_string(spawned);
            SpawnObject(name, "islandT", "islandT", pos, glm::vec3(4.0f));
            placedPositions.push_back(pos);
            spawned++;
            
        }
        std::cout << "Spawned " << spawned << " random islands" << std::endl;
    }



	// Placement price table (Oil + Fish), keyed by prop tag. Kept in one place so
	// the spawn-panel GUI and the placement handler read identical numbers.
	UnitStats::PricePoints GetPropPrice(const std::string& tag) {
		UnitStats::PricePoints p; // defaults to {0, 0}
		if      (tag == "ship")            { p.oil = 10; p.fish = 20; }
		else if (tag == "enemyShip")       { p.oil = 10; p.fish = 20; }
		else if (tag == "oilRig")          { p.oil = 50; p.fish =  0; }
		else if (tag == "fishBoat")        { p.oil = 10; p.fish =  0; }
		else if (tag == "frigate")         { p.oil = 20; p.fish = 40; }
		else if (tag == "destroyer")       { p.oil = 30; p.fish = 60; }
		else if (tag == "aircraftCarrier") { p.oil = 40; p.fish = 80; }
		else if (tag == "turret")          { p.oil = 15; p.fish = 30; }
		// aircraft, mine, bases: free (0/0).
		return p;
	}

	UnitStats SceneManager::InitializeUnitsStats(SceneObject* object) {
		
		std::string tag = object->GetTag();
        UnitStats stats= object->unitStats;

        // Placement cost comes from the shared price table (single source of
        // truth shared with the spawn-panel GUI).
        stats.price = GetPropPrice(tag);
        if (tag == "ship")
        {
            stats.health = 120;
            stats.attack = 18;
            stats.maxHealth = 120;
            stats.attackRange = 90.0f;
            stats.isCombatUnit = true;
            stats.isAlive = true;
            stats.isMovable = true;
            stats.faction = 1;
            stats.attackMode = AttackMode::Projectile;
        }
        else if (tag == "enemyShip")
        {
            stats.health = 120;
            stats.attack = 18;
            stats.maxHealth = 120;
            stats.attackRange = 190.0f;
            stats.isCombatUnit = true;
            stats.isAlive = true;
            stats.isMovable = true;
            stats.faction = 2;
            stats.attackMode = AttackMode::Projectile;
        }
        else if (tag == "oilRig")
        {
            stats.health = 200;
            stats.attack = 0;
            stats.maxHealth = 200;
            stats.attackRange = 0.0f;
            stats.isCombatUnit = false;
            stats.isAlive = true;
            stats.resourceType = "Oil";
            stats.productionRate = 5.0f;
            stats.isMovable = false;
        }
        else if (tag == "fishBoat")
        {
            stats.health = 200;
            stats.attack = 0;
            stats.maxHealth = 200;
            stats.attackRange = 0.0f;
            stats.isCombatUnit = false;
            stats.isAlive = true;
            stats.resourceType = "Fish";
            stats.productionRate = 5.0f;
            stats.isMovable = true;
            stats.faction = 1;
        }
        else if (tag == "frigate")
        {
            stats.health = 260;
            stats.attack = 30;
            stats.maxHealth = 260;
            stats.attackRange = 420.0f;
            stats.isCombatUnit = true;
            stats.isAlive = true;
            stats.isMovable = true;
            stats.faction = 1;
            stats.attackMode = AttackMode::Projectile;
		}
        else if (tag == "destroyer")
        {

            stats.health = 600;
            stats.attack = 60;
            stats.maxHealth = 600;
            stats.attackRange = 280.0f;
            stats.baseAttackCooldown = 2.0f; // 60 dmg / 2s = 30 DPS, AOE
            stats.isCombatUnit = true;
            stats.isAlive = true;
            stats.isMovable = true;
            stats.faction = 1;
            stats.attackMode = AttackMode::ProjectileSplash;
            stats.splashRadius = 20.0f;
        }
        else if(tag =="aircraftCarrier")
        {
            stats.health = 1200;
            stats.attack = 0;
            stats.maxHealth = 1200;
            stats.attackRange = 420.0f;
            stats.isCombatUnit = true;
            stats.isAlive = true;
            stats.isMovable = true;
            stats.faction = 1;
        }
        else if (tag == "aircraft")
        {
            stats.health = 80;
            stats.attack = 45;
            stats.maxHealth = 80;
            stats.attackRange = 150.0f;
            stats.isCombatUnit = true;
            stats.isAlive = true;
            stats.isMovable = false;
            stats.faction = 1;
            stats.attackMode = AttackMode::ProjectileSplash;
            stats.splashRadius = 60.0f;
        }
        else if (tag == "turret")
        {
            stats.health = 350;
            stats.attack = 5;
            stats.maxHealth = 350;
            stats.attackRange = 420.0f;
            stats.isCombatUnit = true;
            stats.isAlive = true;
            stats.isMovable = false;
            stats.faction = 2;
            stats.attackMode = AttackMode::Projectile;
            stats.baseAttackCooldown = 0.1f; // ~10 rounds/sec, 50 DPS
        }
        else if (tag == "mine")
        {
            stats.health       = 1000;
            stats.maxHealth    = 1000;
            stats.attack       = 0;
            stats.attackRange  = 0.0f;
            stats.faction = 1;
            stats.isCombatUnit = false;
            stats.isAlive      = true;
            stats.isMovable    = false;
            // faction is overridden by m_propPlacementFaction in main.cpp's
        }
        else if (tag == "baseP1" || tag == "baseP2")
        {
            // Player HQ: huge HP pool, immobile, non-combat. Loss = match over.
            stats.health = 5000;
            stats.maxHealth = 5000;
            stats.attack = 0;
            stats.attackRange = 0.0f;
            stats.isCombatUnit = false;
            stats.isAlive = true;
            stats.isMovable = false;
            stats.faction = (tag == "baseP1") ? 1 : 2;
        }
        return stats;
	}

}