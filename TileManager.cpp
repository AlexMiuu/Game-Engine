//
// TileManager.cpp
// Implementarea TileManager REFACTORIZAT pentru noul SceneObject system
//

#include "TileManager.hpp"
#include "Scene.hpp"
#include "SceneObject.hpp"
#include "Model3D.hpp"
#include <iostream>
#include <cmath>
#include <limits>
#include <random>

namespace gps {

    // ===========================
    // TILE EFFECT TABLE
    // ===========================
    const TileEffect& GetTileEffect(TileType t) {
        static const TileEffect kSea      { 1.0f, "none", 1.0f };
        static const TileEffect kOil      { 1.0f, "Oil",  2.0f };
        static const TileEffect kFish     { 1.0f, "Fish", 2.0f };
        static const TileEffect kShallows { 2.0f, "none", 1.0f }; // 2x duration = half speed
        static const TileEffect kLand     { 1.0f, "none", 1.0f };
        switch (t) {
            case TileType::Oil:      return kOil;
            case TileType::Fish:     return kFish;
            case TileType::Shallows: return kShallows;
            case TileType::Land:     return kLand;
            case TileType::Sea:
            default:                 return kSea;
        }
    }

    TileManager::TileManager(float tileSize, glm::vec3 model_size, int height)
        : m_terrainModel(nullptr)
        , m_scene(nullptr)
        , m_tileSize(tileSize)
        , m_tileModelScale(model_size)
		, heightTile(height) 
    {
    }

    // ===========================
    // INITIALIZATION
    // ===========================

    void TileManager::Initialize(Model3D* terrainModel, Scene* scene) {
        m_terrainModel = terrainModel;
        m_scene = scene;

        std::cout << "[TileManager] Initialized with tile size: " << m_tileSize;
        if (m_scene) {
            std::cout << " (scene: " << m_scene->GetName() << ")";
        }
        std::cout << std::endl;
    }

    void TileManager::RegisterTileModel(TileType type, Model3D* model) {
        if (!model) return;
        m_typeModels[static_cast<int>(type)] = model;
    }

    // ===========================
    // TILE CREATION / LOADING
    // ===========================

    Tile* TileManager::CreateTile(int gridX, int gridZ, const std::string& terrainType) {
        GridKey key = { gridX, gridZ };

        if (m_tiles.find(key) != m_tiles.end()) {
            return &m_tiles[key];
        }

        glm::vec3 worldPos = GridToWorld(gridX, gridZ);
        Tile newTile(gridX, gridZ, worldPos, terrainType);
        m_tiles[key] = newTile;

        return &m_tiles[key];
    }

    bool TileManager::LoadTile(int gridX, int gridZ) {
        if (!m_scene) {
            std::cerr << "[TileManager] ERROR: No scene set! Call Initialize() first." << std::endl;
            return false;
        }

        if (!m_terrainModel) {
            std::cerr << "[TileManager] ERROR: No terrain model set!" << std::endl;
            return false;
        }

        GridKey key = { gridX, gridZ };

        // Creeaza tile-ul daca nu exista
        auto it = m_tiles.find(key);
        if (it == m_tiles.end()) {
            CreateTile(gridX, gridZ);
            it = m_tiles.find(key);
        }

        Tile& tile = it->second;

        // Skip daca e deja incarcat
        if (tile.isLoaded) {
            return false;
        }

        // ─── Creeaza SceneObject prin Scene ───
        int objID = CreateSceneObjectForTile(tile);

        if (objID < 0) {
            std::cerr << "[TileManager] Failed to create SceneObject for tile ("
                << gridX << ", " << gridZ << ")" << std::endl;
            return false;
        }

        tile.sceneObjectId = objID;
        tile.isLoaded = true;

        std::cout << "[TileManager] Loaded tile (" << gridX << ", " << gridZ
            << ") → SceneObject ID: " << objID << std::endl;

        return true;
    }

    bool TileManager::UnloadTile(int gridX, int gridZ) {
        if (!m_scene) return false;

        GridKey key = { gridX, gridZ };

        auto it = m_tiles.find(key);
        if (it == m_tiles.end() || !it->second.isLoaded) {
            return false;
        }

        Tile& tile = it->second;

        // ─── Sterge SceneObject-ul din scena ───
        m_scene->DestroyObject(tile.sceneObjectId);

        std::cout << "[TileManager] Unloaded tile (" << gridX << ", " << gridZ
            << ") SceneObject ID: " << tile.sceneObjectId << std::endl;

        tile.isLoaded = false;
        tile.sceneObjectId = -1;

        return true;
    }

    // ===========================
    // BATCH OPERATIONS
    // ===========================

    void TileManager::LoadTilesInRadius(const glm::vec3& centerPos, float radius) {
        int centerGridX, centerGridZ;
        WorldToGrid(centerPos, centerGridX, centerGridZ);

        int radiusTiles = static_cast<int>(std::ceil(radius / m_tileSize));

        int loadedCount = 0;
        for (int x = centerGridX - radiusTiles; x <= centerGridX + radiusTiles; ++x) {
            for (int z = centerGridZ - radiusTiles; z <= centerGridZ + radiusTiles; ++z) {
                glm::vec3 tileWorldPos = GridToWorld(x, z);
                float dist = glm::distance(centerPos, tileWorldPos);

                if (dist <= radius) {
                    if (!HasTile(x, z)) {
                        CreateTile(x, z);
                    }
                    if (LoadTile(x, z)) {
                        loadedCount++;
                    }
                }
            }
        }

        if (loadedCount > 0) {
            std::cout << "[TileManager] Loaded " << loadedCount << " tiles in radius " << radius << std::endl;
        }
    }

    void TileManager::UnloadTilesOutsideRadius(const glm::vec3& centerPos, float radius) {
        std::vector<GridKey> tilesToUnload;

        for (auto& pair : m_tiles) {
            if (pair.second.isLoaded) {
                float dist = glm::distance(centerPos, pair.second.worldPos);
                if (dist > radius) {
                    tilesToUnload.push_back(pair.first);
                }
            }
        }

        for (const auto& key : tilesToUnload) {
            UnloadTile(key.x, key.z);
        }
    }

    void TileManager::GenerateGrid(int minX, int maxX, int minZ, int maxZ,
        const std::string& terrainType) {
        for (int x = minX; x <= maxX; ++x) {
            for (int z = minZ; z <= maxZ; ++z) {
                CreateTile(x, z, terrainType);
            }
        }
        std::cout << "[TileManager] Generated grid (" << minX << "," << minZ
            << ") to (" << maxX << "," << maxZ << ") = " << m_tiles.size() << " tiles" << std::endl;
    }

    void TileManager::GenerateAndLoadGrid(int minX, int maxX, int minZ, int maxZ,
        const std::string& terrainType) {
        // 1. create all tiles (no scene objects yet)
        for (int x = minX; x <= maxX; ++x)
            for (int z = minZ; z <= maxZ; ++z)
                CreateTile(x, z, terrainType);

        // 2. assign procedural TileType so LoadTile knows which icon to spawn
        AssignProceduralTypes();

        // 3. load all (creates terrain SceneObjects + spawns icons)
        for (int x = minX; x <= maxX; ++x)
            for (int z = minZ; z <= maxZ; ++z)
                LoadTile(x, z);

        std::cout << "[TileManager] Generated and loaded grid: " << m_tiles.size() << " tiles" << std::endl;
    }

    // ===========================
    // PROCEDURAL TYPE ASSIGNMENT
    // ===========================
    void TileManager::AssignProceduralTypes(unsigned seed) {
        std::mt19937 rng(seed);
        std::uniform_real_distribution<float> roll(0.0f, 1.0f);

        // Density per type (sums < 1.0; remainder stays Sea).
        // Tunable: bump up Oil/Fish to see more icons during dev.
        constexpr float kOil      = 0.15f;
        constexpr float kFish     = 0.15f;
        constexpr float kShallows = 0.10f;
        constexpr float kLand     = 0.10f;

        for (auto& pair : m_tiles) {
            Tile& tile = pair.second;
            // Don't reassign tiles that already have a non-default type or are already loaded.
            if (tile.type != TileType::Sea || tile.isLoaded) continue;

            float r = roll(rng);
            if      (r < kOil)                                tile.type = TileType::Oil;
            else if (r < kOil + kFish)                        tile.type = TileType::Fish;
            else if (r < kOil + kFish + kShallows)            tile.type = TileType::Shallows;
            else if (r < kOil + kFish + kShallows + kLand)    tile.type = TileType::Land;
            // else stays Sea
        }
    }

    Tile* TileManager::GetTile(int gridX, int gridZ) {
        GridKey key = { gridX, gridZ };
        auto it = m_tiles.find(key);
        return (it != m_tiles.end()) ? &it->second : nullptr;
    }

    bool TileManager::HasTile(int gridX, int gridZ) const {
        GridKey key = { gridX, gridZ };
        return m_tiles.find(key) != m_tiles.end();
    }

    SceneObject* TileManager::GetTileSceneObject(int gridX, int gridZ) {
        Tile* tile = GetTile(gridX, gridZ);
        if (!tile || !tile->isLoaded || !m_scene) return nullptr;
        return m_scene->GetObjectByID(tile->sceneObjectId);
    }

    // ---------- Hex grid math ----------
    // Flat-top hexes (vertices at +X / -X), odd-q offset coordinates
    // (odd columns are shifted in +Z by half a row).
    //   m_tileSize = world center-to-vertex distance (the canonical "size" of a hex).
    //   colSpacing = 1.5 * size  (column-to-column horizontal distance)
    //   rowSpacing = sqrt(3) * size  (row-to-row vertical distance, also the flat-edge length doubled... no, just that)

    glm::vec3 TileManager::GridToWorld(int gridX, int gridZ) const {
        const float s = m_tileSize;
        const float colSpacing = 1.5f * s;
        const float rowSpacing = std::sqrt(3.0f) * s;
        const float zShift = (gridX & 1) ? rowSpacing * 0.5f : 0.0f;
        return glm::vec3(gridX * colSpacing, 0.0f, gridZ * rowSpacing + zShift);
    }

    void TileManager::WorldToGrid(const glm::vec3& worldPos, int& outGridX, int& outGridZ) const {
        const float s = m_tileSize;
        // World -> fractional axial (flat-top)
        const float qf = (2.0f / 3.0f) * worldPos.x / s;
        const float rf = ((-1.0f / 3.0f) * worldPos.x + (std::sqrt(3.0f) / 3.0f) * worldPos.z) / s;
        // Cube round (axial -> cube uses x=q, z=r, y=-x-z)
        float x = qf;
        float z = rf;
        float y = -x - z;
        float rx = std::round(x);
        float ry = std::round(y);
        float rz = std::round(z);
        const float xd = std::abs(rx - x);
        const float yd = std::abs(ry - y);
        const float zd = std::abs(rz - z);
        if (xd > yd && xd > zd)      rx = -ry - rz;
        else if (yd > zd)            ry = -rx - rz;
        else                         rz = -rx - ry;
        // Axial -> odd-q offset
        const int col = static_cast<int>(rx);
        const int row = static_cast<int>(rz) + (col - (col & 1)) / 2;
        outGridX = col;
        outGridZ = row;
    }

    std::vector<Tile*> TileManager::GetLoadedTiles() {
        std::vector<Tile*> result;
        for (auto& pair : m_tiles) {
            if (pair.second.isLoaded) {
                result.push_back(&pair.second);
            }
        }
        return result;
    }

    void TileManager::SetTileModelScale(const glm::vec3& scale) {
        m_tileModelScale = scale;

        if (!m_scene) {
            return;
        }

        for (auto& pair : m_tiles) {
            Tile& tile = pair.second;
            if (!tile.isLoaded || tile.sceneObjectId < 0) {
                continue;
            }

            SceneObject* obj = m_scene->GetObjectByID(tile.sceneObjectId);
            if (!obj) {
                continue;
            }

            obj->GetTransform().SetScale(m_tileModelScale);
            obj->UpdateWorldBounds();
        }
    }

    void TileManager::SetTileHeight(int height) {

        for (auto& pair : m_tiles) {
            Tile& tile = pair.second;
            if (!tile.isLoaded || tile.sceneObjectId < 0) {
                continue;
            }

            SceneObject* obj = m_scene->GetObjectByID(tile.sceneObjectId);
            if (!obj) {
                continue;
            }

            obj->GetTransform().SetHeight(height);
            obj->UpdateWorldBounds();
        }

    }
    std::vector<Tile*> TileManager::GetAllTiles() {
        std::vector<Tile*> result;
        for (auto& pair : m_tiles) {
            result.push_back(&pair.second);
        }
        return result;
    }

    int TileManager::GetLoadedCount() const {
        int count = 0;
        for (const auto& pair : m_tiles) {
            if (pair.second.isLoaded) count++;
        }
        return count;
    }

    bool TileManager::GetGridBounds(glm::vec3& outMin, glm::vec3& outMax) const {
        if (m_tiles.empty()) return false;

        int minX = std::numeric_limits<int>::max();
        int maxX = std::numeric_limits<int>::lowest();
        int minZ = std::numeric_limits<int>::max();
        int maxZ = std::numeric_limits<int>::lowest();

        bool hasLoaded = false;
        for (const auto& pair : m_tiles) {
            if (!pair.second.isLoaded) continue;
            hasLoaded = true;
            if (pair.first.x < minX) minX = pair.first.x;
            if (pair.first.x > maxX) maxX = pair.first.x;
            if (pair.first.z < minZ) minZ = pair.first.z;
            if (pair.first.z > maxZ) maxZ = pair.first.z;
        }

        if (!hasLoaded) return false;

        // World-space AABB enclosing all loaded hex tiles.
        // Flat-top hex extents: ±size on X (vertex), ±sqrt(3)/2 * size on Z (edge midpoint).
        const float s = m_tileSize;
        const float halfW = s;
        const float halfH = std::sqrt(3.0f) * 0.5f * s;
        glm::vec3 cMin = GridToWorld(minX, minZ);
        glm::vec3 cMax = GridToWorld(maxX, maxZ);
        // Account for the row-shift on odd columns by widening the Z range by half a row.
        const float rowShift = std::sqrt(3.0f) * 0.5f * s;
        outMin = glm::vec3(cMin.x - halfW, -1000.0f, std::min(cMin.z, cMax.z) - halfH - rowShift);
        outMax = glm::vec3(cMax.x + halfW,  1000.0f, std::max(cMin.z, cMax.z) + halfH + rowShift);
        return true;
    }

    void TileManager::GetGridRange(int& outMinX, int& outMaxX, int& outMinZ, int& outMaxZ) const {
        outMinX = 0; outMaxX = 0; outMinZ = 0; outMaxZ = 0;
        bool first = true;
        for (const auto& pair : m_tiles) {
            if (first) {
                outMinX = outMaxX = pair.first.x;
                outMinZ = outMaxZ = pair.first.z;
                first = false;
            } else {
                if (pair.first.x < outMinX) outMinX = pair.first.x;
                if (pair.first.x > outMaxX) outMaxX = pair.first.x;
                if (pair.first.z < outMinZ) outMinZ = pair.first.z;
                if (pair.first.z > outMaxZ) outMaxZ = pair.first.z;
            }
        }
    }

    // ===========================
    // CLEANUP
    // ===========================

    void TileManager::Clear() {
        // Descarca toate tile-urile din scena
        UnloadAll();

        // Sterge datele interne
        m_tiles.clear();
        std::cout << "[TileManager] Cleared all tiles" << std::endl;
    }

    void TileManager::UnloadAll() {
        if (!m_scene) return;

        for (auto& pair : m_tiles) {
            Tile& tile = pair.second;
            if (tile.isLoaded) {
                m_scene->DestroyObject(tile.sceneObjectId);
                tile.isLoaded = false;
                tile.sceneObjectId = -1;
            }
        }
        std::cout << "[TileManager] Unloaded all tiles" << std::endl;
    }

    // ===========================
    // DEBUG
    // ===========================

    void TileManager::PrintDebugInfo() const {
        std::cout << "\n=== TileManager Debug Info ===" << std::endl;
        std::cout << "Tile Size: " << m_tileSize << std::endl;
        std::cout << "Total Tiles: " << m_tiles.size() << std::endl;
        std::cout << "Scene: " << (m_scene ? m_scene->GetName() : "NULL") << std::endl;
        std::cout << "Terrain Model: " << (m_terrainModel ? "set" : "NULL") << std::endl;

        int loadedCount = 0;
        for (const auto& pair : m_tiles) {
            if (pair.second.isLoaded) loadedCount++;
        }

        std::cout << "Loaded Tiles: " << loadedCount << std::endl;
        std::cout << "==============================\n" << std::endl;
    }

    // ===========================
    // PRIVATE HELPERS
    // ===========================

    int TileManager::CreateSceneObjectForTile(const Tile& tile) {
        if (!m_scene || !m_terrainModel) return -1;

        // Pick the model registered for this TileType, fall back to terrain default
        Model3D* model = m_terrainModel;
        auto mit = m_typeModels.find(static_cast<int>(tile.type));
        if (mit != m_typeModels.end() && mit->second) model = mit->second;

        // Creeaza un SceneObject real in scena
        std::string tileName = "Tile_" + std::to_string(tile.gridX) + "_" + std::to_string(tile.gridZ);
        SceneObject* obj = m_scene->CreateObject(tileName);

        if (!obj) return -1;

        // Seteaza modelul
        obj->SetModel(model);

        // Seteaza transform
        obj->GetTransform().SetPosition(tile.worldPos);
        obj->GetTransform().SetScale(m_tileModelScale);
        obj->GetTransform().SetHeight(heightTile);
        obj->SetTag("tile");
        obj->unitStats.faction= -1;
        obj->unitStats.isAlive = false;
        // Calculeaza bounding sphere
        glm::vec3 center;
        float radius;
        ComputeLocalBoundingSphere(model, center, radius);
        obj->SetLocalBounds(center, radius);
        obj->UpdateWorldBounds();

        return obj->GetID();
    }

    void TileManager::ComputeLocalBoundingSphere(Model3D* model, glm::vec3& outCenter, float& outRadius) {
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