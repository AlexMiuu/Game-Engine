#include "TileManager.hpp"
#include <iostream>
#include <cmath>
// CORECTARE: Nu mai include Model3D.hpp aici (e deja în TileManager.hpp)

namespace gps {

    TileManager::TileManager(float tileSize)
        : m_terrainModel(nullptr), m_tileSize(tileSize) {
    }

    void TileManager::Initialize(Model3D* terrainModel) {
        m_terrainModel = terrainModel;
        std::cout << "[TileManager] Initialized with tile size: " << m_tileSize << std::endl;
    }

    Tile* TileManager::CreateTile(int gridX, int gridZ, const std::string& terrainType) {
        GridKey key = { gridX, gridZ };

        // Verifica daca tile-ul exista deja
        if (m_tiles.find(key) != m_tiles.end()) {
            std::cout << "[TileManager] Tile at (" << gridX << ", " << gridZ
                << ") already exists" << std::endl;
            return &m_tiles[key];
        }

        // Calculeaza pozitia in world space
        glm::vec3 worldPos = GridToWorld(gridX, gridZ);

        // Creeaza tile-ul nou
        Tile newTile(gridX, gridZ, worldPos, terrainType);
        m_tiles[key] = newTile;

        std::cout << "[TileManager] Created tile at grid (" << gridX << ", " << gridZ
            << ") world pos (" << worldPos.x << ", " << worldPos.y << ", "
            << worldPos.z << ")" << std::endl;

        return &m_tiles[key];
    }

    // CORECTARE: Actualizat sã foloseascã LegacySceneObject în loc de SceneObject
    bool TileManager::LoadTile(int gridX, int gridZ,
        std::vector<LegacySceneObject>& sceneObjects, int& nextID) {
        GridKey key = { gridX, gridZ };

        // Verifica daca tile-ul exista
        auto it = m_tiles.find(key);
        if (it == m_tiles.end()) {
            std::cout << "[TileManager] Tile at (" << gridX << ", " << gridZ
                << ") does not exist. Creating it..." << std::endl;
            CreateTile(gridX, gridZ);
            it = m_tiles.find(key);
        }

        Tile& tile = it->second;

        // Verifica daca e deja incarcat
        if (tile.isLoaded) {
            std::cout << "[TileManager] Tile at (" << gridX << ", " << gridZ
                << ") is already loaded" << std::endl;
            return false;
        }

        // Verifica daca avem model
        if (!m_terrainModel) {
            std::cerr << "[TileManager] ERROR: No terrain model set!" << std::endl;
            return false;
        }

        // CORECTARE: Acum returneazã LegacySceneObject
        LegacySceneObject obj = CreateSceneObjectForTile(tile, nextID);
        sceneObjects.push_back(obj);

        // Actualizeaza tile
        tile.sceneObjectId = nextID;
        tile.isLoaded = true;
        nextID++;

        std::cout << "[TileManager] Loaded tile at grid (" << gridX << ", " << gridZ
            << ") with ID " << tile.sceneObjectId << std::endl;

        return true;
    }

    // CORECTARE: Actualizat sã foloseascã LegacySceneObject
    bool TileManager::UnloadTile(int gridX, int gridZ,
        std::vector<LegacySceneObject>& sceneObjects) {
        GridKey key = { gridX, gridZ };

        auto it = m_tiles.find(key);
        if (it == m_tiles.end() || !it->second.isLoaded) {
            return false;
        }

        Tile& tile = it->second;
        int objectId = tile.sceneObjectId;

        // Sterge din sceneObjects
        for (auto objIt = sceneObjects.begin(); objIt != sceneObjects.end(); ++objIt) {
            if (objIt->id == objectId) {
                sceneObjects.erase(objIt);
                std::cout << "[TileManager] Unloaded tile at grid (" << gridX << ", "
                    << gridZ << ") with ID " << objectId << std::endl;
                break;
            }
        }

        // Actualizeaza tile
        tile.isLoaded = false;
        tile.sceneObjectId = -1;

        return true;
    }

    Tile* TileManager::GetTile(int gridX, int gridZ) {
        GridKey key = { gridX, gridZ };
        auto it = m_tiles.find(key);

        if (it != m_tiles.end()) {
            return &it->second;
        }

        return nullptr;
    }

    bool TileManager::HasTile(int gridX, int gridZ) const {
        GridKey key = { gridX, gridZ };
        return m_tiles.find(key) != m_tiles.end();
    }

    // CORECTARE: Actualizat sã foloseascã LegacySceneObject
    void TileManager::LoadTilesInRadius(const glm::vec3& centerPos, float radius,
        std::vector<LegacySceneObject>& sceneObjects, int& nextID) {
        // Converti pozitia centrala la grid
        int centerGridX, centerGridZ;
        WorldToGrid(centerPos, centerGridX, centerGridZ);

        // Calculeaza range-ul de tile-uri
        int radiusTiles = static_cast<int>(std::ceil(radius / m_tileSize));

        // Incarca toate tile-urile in raza
        for (int x = centerGridX - radiusTiles; x <= centerGridX + radiusTiles; ++x) {
            for (int z = centerGridZ - radiusTiles; z <= centerGridZ + radiusTiles; ++z) {
                // Verifica distanta
                glm::vec3 tileWorldPos = GridToWorld(x, z);
                float dist = glm::distance(centerPos, tileWorldPos);

                if (dist <= radius) {
                    // Creeaza tile daca nu exista
                    if (!HasTile(x, z)) {
                        CreateTile(x, z);
                    }

                    // Incarca tile
                    LoadTile(x, z, sceneObjects, nextID);
                }
            }
        }
    }

    // CORECTARE: Actualizat sã foloseascã LegacySceneObject
    void TileManager::UnloadTilesOutsideRadius(const glm::vec3& centerPos, float radius,
        std::vector<LegacySceneObject>& sceneObjects) {
        std::vector<GridKey> tilesToUnload;

        // Gaseste toate tile-urile care sunt in afara razei
        for (auto& pair : m_tiles) {
            if (pair.second.isLoaded) {
                glm::vec3 tilePos = pair.second.worldPos;
                float dist = glm::distance(centerPos, tilePos);

                if (dist > radius) {
                    tilesToUnload.push_back(pair.first);
                }
            }
        }

        // Descarca tile-urile
        for (const auto& key : tilesToUnload) {
            UnloadTile(key.x, key.z, sceneObjects);
        }
    }

    void TileManager::GenerateGrid(int minX, int maxX, int minZ, int maxZ,
        const std::string& terrainType) {
        std::cout << "[TileManager] Generating grid from (" << minX << "," << minZ
            << ") to (" << maxX << "," << maxZ << ")" << std::endl;

        for (int x = minX; x <= maxX; ++x) {
            for (int z = minZ; z <= maxZ; ++z) {
                CreateTile(x, z, terrainType);
            }
        }

        std::cout << "[TileManager] Generated " << m_tiles.size() << " tiles" << std::endl;
    }

    void TileManager::WorldToGrid(const glm::vec3& worldPos, int& outGridX, int& outGridZ) const {
        outGridX = static_cast<int>(std::floor(worldPos.x / m_tileSize));
        outGridZ = static_cast<int>(std::floor(worldPos.z / m_tileSize));
    }

    glm::vec3 TileManager::GridToWorld(int gridX, int gridZ) const {
        // Centreaza tile-ul la pozitia grid
        float worldX = gridX * m_tileSize + (m_tileSize * 0.5f);
        float worldZ = gridZ * m_tileSize + (m_tileSize * 0.5f);

        return glm::vec3(worldX, 0.0f, worldZ);
    }

    std::vector<Tile*> TileManager::GetLoadedTiles() {
        std::vector<Tile*> loadedTiles;

        for (auto& pair : m_tiles) {
            if (pair.second.isLoaded) {
                loadedTiles.push_back(&pair.second);
            }
        }

        return loadedTiles;
    }

    std::vector<Tile*> TileManager::GetAllTiles() {
        std::vector<Tile*> allTiles;

        for (auto& pair : m_tiles) {
            allTiles.push_back(&pair.second);
        }

        return allTiles;
    }

    void TileManager::Clear() {
        m_tiles.clear();
        std::cout << "[TileManager] Cleared all tiles" << std::endl;
    }

    void TileManager::PrintDebugInfo() const {
        std::cout << "\n=== TileManager Debug Info ===" << std::endl;
        std::cout << "Tile Size: " << m_tileSize << std::endl;
        std::cout << "Total Tiles: " << m_tiles.size() << std::endl;

        int loadedCount = 0;
        for (const auto& pair : m_tiles) {
            if (pair.second.isLoaded) loadedCount++;
        }

        std::cout << "Loaded Tiles: " << loadedCount << std::endl;
        std::cout << "==============================\n" << std::endl;
    }

    // CORECTARE: Returneazã LegacySceneObject cu constructor implicit valid
    LegacySceneObject TileManager::CreateSceneObjectForTile(const Tile& tile, int objectId) {
        LegacySceneObject obj;  // Acum func?ioneazã - LegacySceneObject are constructor implicit
        obj.id = objectId;
        obj.modelPtr = m_terrainModel;

        // Creeaza matricea de transformare pentru tile
        glm::mat4 T = glm::translate(glm::mat4(1.0f), tile.worldPos);
        obj.modelMatrix = T;
        obj.scale = glm::vec3(1.0f);

        // Calculeaza bounding sphere (simplificat)
        // In mod normal ai vrea sa apelezi computeLocalBoundingSphere
        obj.localCenter = glm::vec3(0.0f);
        obj.localRadius = m_tileSize * 0.5f;

        // Calculeaza world bounds
        // CORECTARE: Constructor corect pentru glm::vec4 (4 parametri)
        glm::vec4 c = obj.modelMatrix * glm::vec4(obj.localCenter.x, obj.localCenter.y, obj.localCenter.z, 1.0f);
        obj.worldCenter = glm::vec3(c.x, c.y, c.z);
        obj.worldRadius = obj.localRadius;

        return obj;
    }

} // namespace gps