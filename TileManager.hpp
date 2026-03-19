//
// TileManager.hpp
// Manager de tile-uri REFACTORIZAT pentru noul SceneObject system
//
// SCHIMBARI vs. versiunea veche:
//   - Eliminat LegacySceneObject complet
//   - Foloseste Scene::CreateObject() / Scene::DestroyObject()
//   - Tile-urile sunt acum SceneObject-uri reale in scena
//   - Compatibil cu SelectionSystem, rendering pipeline, etc.
//

#ifndef TILE_MANAGER_HPP
#define TILE_MANAGER_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <map>
#include <vector>
#include <string>

// Forward declarations (evitam dependinte circulare)
namespace gps {
    class Model3D;
    class Scene;
    class SceneObject;
}

namespace gps {

    // ===========================
    // TILE STRUCT
    // ===========================
    struct Tile {
        int gridX;              // Coordonata X in grid
        int gridZ;              // Coordonata Z in grid
        glm::vec3 worldPos;     // Pozitia in world space
        int sceneObjectId;      // ID-ul SceneObject-ului din Scene (NOU!)
        bool isLoaded;          // Daca e incarcat in scena
        std::string terrainType;

        Tile() : gridX(0), gridZ(0), worldPos(0.0f), sceneObjectId(-1),
            isLoaded(false), terrainType("default") {}

        Tile(int x, int z, const glm::vec3& pos, const std::string& type = "default")
            : gridX(x), gridZ(z), worldPos(pos), sceneObjectId(-1),
            isLoaded(false), terrainType(type) {}
    };

    // ===========================
    // TILE MANAGER (REFACTORIZAT)
    // ===========================
    class TileManager {
    public:
        TileManager(float tileSize = 300.0f);

        // ===========================
        // INITIALIZATION
        // ===========================

        /**
         * @brief Initializeaza TileManager cu model de teren SI scena
         * @param terrainModel Modelul 3D folosit pentru tile-uri
         * @param scene Pointer la scena (pentru a crea SceneObject-uri)
         *
         * NOTA: scene NU mai e optional - TileManager are nevoie de el
         */
        void Initialize(Model3D* terrainModel, Scene* scene);

        // ===========================
        // TILE CREATION / LOADING
        // ===========================

        /**
         * @brief Creeaza un tile la coordonatele grid specificate
         * NU il incarca in scena - doar il inregistreaza intern
         */
        Tile* CreateTile(int gridX, int gridZ, const std::string& terrainType = "default");

        /**
         * @brief Incarca un tile in scena (creeaza SceneObject)
         * @return true daca tile-ul a fost incarcat cu succes
         *
         * Creeaza un SceneObject real prin Scene::CreateObject()
         * cu model, transform si bounding sphere setate corect.
         */
        bool LoadTile(int gridX, int gridZ);

        /**
         * @brief Descarca un tile din scena (sterge SceneObject-ul)
         * @return true daca tile-ul a fost descarcat cu succes
         */
        bool UnloadTile(int gridX, int gridZ);

        // ===========================
        // BATCH OPERATIONS
        // ===========================

        /**
         * @brief Incarca tile-urile intr-o raza specificata
         */
        void LoadTilesInRadius(const glm::vec3& centerPos, float radius);

        /**
         * @brief Descarca tile-urile in afara razei
         */
        void UnloadTilesOutsideRadius(const glm::vec3& centerPos, float radius);

        /**
         * @brief Genereaza un grid de tile-uri (doar creare, fara load)
         */
        void GenerateGrid(int minX, int maxX, int minZ, int maxZ,
            const std::string& terrainType = "default");

        /**
         * @brief Genereaza SI incarca un grid complet
         * Shortcut pentru GenerateGrid() + LoadAll()
         */
        void GenerateAndLoadGrid(int minX, int maxX, int minZ, int maxZ,
            const std::string& terrainType = "default");

        // ===========================
        // QUERIES
        // ===========================

        Tile* GetTile(int gridX, int gridZ);
        bool HasTile(int gridX, int gridZ) const;

        /**
         * @brief Obtine SceneObject-ul asociat unui tile
         * @return Pointer la SceneObject sau nullptr
         */
        SceneObject* GetTileSceneObject(int gridX, int gridZ);

        // Conversii coordonate
        void WorldToGrid(const glm::vec3& worldPos, int& outGridX, int& outGridZ) const;
        glm::vec3 GridToWorld(int gridX, int gridZ) const;

        // Getters
        float GetTileSize() const { return m_tileSize; }
        void SetTileSize(float size) { m_tileSize = size; }

        void SetTileHeight();
        int GetTileHeight() { return heightTile; }

        const glm::vec3& GetTileModelScale() const { return m_tileModelScale; }
        void SetTileModelScale(const glm::vec3& scale);

        std::vector<Tile*> GetLoadedTiles();
        void SetTileHeight(int height);

        std::vector<Tile*> GetAllTiles();
        int GetLoadedCount() const;
        int GetTotalCount() const { return static_cast<int>(m_tiles.size()); }

        // Grid bounds (returns the world-space AABB of all loaded tiles)
        bool GetGridBounds(glm::vec3& outMin, glm::vec3& outMax) const;

        // Grid range getters
        void GetGridRange(int& outMinX, int& outMaxX, int& outMinZ, int& outMaxZ) const;

        // ===========================
        // CLEANUP
        // ===========================

        /**
         * @brief Sterge toate tile-urile (si SceneObject-urile din scena)
         */
        void Clear();

        /**
         * @brief Descarca toate tile-urile fara a le sterge
         */
        void UnloadAll();

        // ===========================
        // DEBUG
        // ===========================
        void PrintDebugInfo() const;

    private:
        struct GridKey {
            int x, z;

            bool operator<(const GridKey& other) const {
                if (x != other.x) return x < other.x;
                return z < other.z;
            }

            bool operator==(const GridKey& other) const {
                return x == other.x && z == other.z;
            }
        };

        std::map<GridKey, Tile> m_tiles;
        Model3D* m_terrainModel;
        Scene* m_scene;          
        float m_tileSize;
        glm::vec3 m_tileModelScale;
        
        int heightTile;

        /**
         * @brief Creeaza un SceneObject pentru un tile si il adauga in scena
         * @return ID-ul SceneObject-ului creat, sau -1 la eroare
         */
        int CreateSceneObjectForTile(const Tile& tile);

        /**
         * @brief Calculeaza bounding sphere local pentru model
         */
        void ComputeLocalBoundingSphere(Model3D* model, glm::vec3& outCenter, float& outRadius);
    };

} // namespace gps

#endif // TILE_MANAGER_HPP