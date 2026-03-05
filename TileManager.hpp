#ifndef TILE_MANAGER_HPP
#define TILE_MANAGER_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <map>
#include <vector>
#include <string>
#include "Model3D.hpp"

namespace gps {

    // Structura pentru un tile individual
    struct Tile {
        int gridX;           // Coordonata X in grid
        int gridZ;           // Coordonata Z in grid
        glm::vec3 worldPos;  // Pozitia in world space
        int sceneObjectId;   // ID-ul in g_sceneObjects
        bool isLoaded;       // Daca e incarcat in scena
        std::string terrainType; // Tipul de teren (ex: "grass", "desert", "snow")

        Tile() : gridX(0), gridZ(0), worldPos(0.0f), sceneObjectId(-1),
            isLoaded(false), terrainType("default") {}

        Tile(int x, int z, const glm::vec3& pos, const std::string& type = "default")
            : gridX(x), gridZ(z), worldPos(pos), sceneObjectId(-1),
            isLoaded(false), terrainType(type) {}
    };

    // CORECTARE: Redenumit din SceneObject în LegacySceneObject
    // pentru a evita conflictul cu clasa SceneObject din SceneObject.hpp
    // Aceastã structurã e folositã doar pentru compatibilitate cu codul vechi
    struct LegacySceneObject {
        int id;
        gps::Model3D* modelPtr;
        glm::mat4 modelMatrix;
        glm::vec3 localCenter;
        float localRadius;
        glm::vec3 worldCenter;
        float worldRadius;
        bool isMoving = false;
        float moveStartTime = 0.0f;
        float moveDuration = 0.0f;
        glm::vec3 moveStartPos;
        glm::vec3 moveEndPos;
        glm::vec3 scale = glm::vec3(1.0f);
    };

    // Managerul de tile-uri
    class TileManager {
    public:
        // Constructor
        TileManager(float tileSize = 300.0f);

        // Initializare cu model de teren
        void Initialize(Model3D* terrainModel);

        // Creaza un tile la coordonatele grid specificate
        Tile* CreateTile(int gridX, int gridZ, const std::string& terrainType = "default");

        // CORECTARE: Actualizat sã foloseascã LegacySceneObject
        // Incarca un tile in scena (adauga in g_sceneObjects)
        bool LoadTile(int gridX, int gridZ, std::vector<LegacySceneObject>& sceneObjects, int& nextID);

        // Descarca un tile din scena
        bool UnloadTile(int gridX, int gridZ, std::vector<LegacySceneObject>& sceneObjects);

        // Obtine tile la coordonate grid
        Tile* GetTile(int gridX, int gridZ);

        // Verifica daca exista tile la coordonate
        bool HasTile(int gridX, int gridZ) const;

        // Incarca tile-urile intr-o raza specificata de la o pozitie
        void LoadTilesInRadius(const glm::vec3& centerPos, float radius,
            std::vector<LegacySceneObject>& sceneObjects, int& nextID);

        // Descarca tile-urile care sunt in afara razei
        void UnloadTilesOutsideRadius(const glm::vec3& centerPos, float radius,
            std::vector<LegacySceneObject>& sceneObjects);

        // Genereaza un grid de tile-uri (pentru initializare)
        void GenerateGrid(int minX, int maxX, int minZ, int maxZ,
            const std::string& terrainType = "default");

        // Converti coordonate world la grid
        void WorldToGrid(const glm::vec3& worldPos, int& outGridX, int& outGridZ) const;

        // Converti coordonate grid la world
        glm::vec3 GridToWorld(int gridX, int gridZ) const;

        // Obtine dimensiunea unui tile
        float GetTileSize() const { return m_tileSize; }

        // Seteaza dimensiunea unui tile
        void SetTileSize(float size) { m_tileSize = size; }

        // Obtine toate tile-urile incarcate
        std::vector<Tile*> GetLoadedTiles();

        // Obtine toate tile-urile (incarcate sau nu)
        std::vector<Tile*> GetAllTiles();

        // Sterge toate tile-urile
        void Clear();

        // Debug: afiseaza informatii despre tile-uri
        void PrintDebugInfo() const;

    private:
        // Hash-ul pentru cheia de grid
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

        // Map-ul cu toate tile-urile
        std::map<GridKey, Tile> m_tiles;

        // Modelul de teren folosit pentru tile-uri
        Model3D* m_terrainModel;

        // Dimensiunea unui tile
        float m_tileSize;

        // CORECTARE: Actualizat sã returneze LegacySceneObject
        // Helper: creeaza un SceneObject pentru un tile
        LegacySceneObject CreateSceneObjectForTile(const Tile& tile, int objectId);
    };

} // namespace gps

#endif // TILE_MANAGER_HPP