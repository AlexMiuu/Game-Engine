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

namespace gps {

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
        for (int x = minX; x <= maxX; ++x) {
            for (int z = minZ; z <= maxZ; ++z) {
                CreateTile(x, z, terrainType);
                LoadTile(x, z);
            }
        }
        std::cout << "[TileManager] Generated and loaded grid: " << m_tiles.size() << " tiles" << std::endl;
    }

    // ===========================
    // QUERIES
    // ===========================

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

    void TileManager::WorldToGrid(const glm::vec3& worldPos, int& outGridX, int& outGridZ) const {
        outGridX = static_cast<int>(std::floor(worldPos.x / m_tileSize));
        outGridZ = static_cast<int>(std::floor(worldPos.z / m_tileSize));
    }

    glm::vec3 TileManager::GridToWorld(int gridX, int gridZ) const {
        float worldX = gridX * m_tileSize + (m_tileSize * 0.5f);
        float worldZ = gridZ * m_tileSize + (m_tileSize * 0.5f);
        return glm::vec3(worldX, 0.0f, worldZ);
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

        // World-space bounds: from the left edge of minX tile to the right edge of maxX tile
        outMin = glm::vec3(minX * m_tileSize, -1000.0f, minZ * m_tileSize);
        outMax = glm::vec3((maxX + 1) * m_tileSize, 1000.0f, (maxZ + 1) * m_tileSize);
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

        // Creeaza un SceneObject real in scena
        std::string tileName = "Tile_" + std::to_string(tile.gridX) + "_" + std::to_string(tile.gridZ);
        SceneObject* obj = m_scene->CreateObject(tileName);

        if (!obj) return -1;

        // Seteaza modelul
        obj->SetModel(m_terrainModel);

        // Seteaza transform
        obj->GetTransform().SetPosition(tile.worldPos);
        obj->GetTransform().SetScale(m_tileModelScale);
        obj->GetTransform().SetHeight(heightTile);
        obj->unitStats.faction= -1;
        obj->unitStats.isAlive = false;
        // Calculeaza bounding sphere
        glm::vec3 center;
        float radius;
        ComputeLocalBoundingSphere(m_terrainModel, center, radius);
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