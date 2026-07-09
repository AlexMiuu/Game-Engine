#ifndef TILE_MANAGER_HPP
#define TILE_MANAGER_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>

namespace gps {
    class Model3D;
    class Scene;
    class SceneObject;
}

namespace gps {

    enum class TileType {
        Sea = 0,    // default, no gimmick
        Oil,        // boosts Oil extractors parked on top
        Fish,       // boosts Fish extractors parked on top
        Shallows,   // slows ships passing through
        Land,       // hosts an island prop, blocks ship movement (future)
    };

    struct TileEffect {
        float moveDurationMul = 1.0f;   // applied to ship moveDuration (>1 = slower)
        std::string resource = "none";  // resource produced by parked extractors
        float productionBonus = 1.0f;   // multiplier on extractor productionRate
    };

    const TileEffect& GetTileEffect(TileType t);

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

        TileType type = TileType::Sea;
        bool isBorder = false;   // decorative half-scale ring around playable area

        Tile() : gridX(0), gridZ(0), worldPos(0.0f), sceneObjectId(-1),
            isLoaded(false), terrainType("default") {}

        Tile(int x, int z, const glm::vec3& pos, const std::string& type = "default")
            : gridX(x), gridZ(z), worldPos(pos), sceneObjectId(-1),
            isLoaded(false), terrainType(type) {}
    };

    class TileManager {
    public:
        TileManager(float tileSize = 257.0f, glm::vec3 model_size= glm::vec3(27.0f), int height=-60);

        void Initialize(Model3D* terrainModel, Scene* scene);
        void RegisterTileModel(TileType type, Model3D* model);
        void SetTileType(int gridX, int gridZ, TileType newType);
        void StampIslandAt(int gridX, int gridZ);

        void AssignProceduralTypes(unsigned seed = 0u);
        void GenerateBorderRing();

        Tile* CreateTile(int gridX, int gridZ, const std::string& terrainType = "default");
        bool LoadTile(int gridX, int gridZ);
        bool UnloadTile(int gridX, int gridZ);


        void LoadTilesInRadius(const glm::vec3& centerPos, float radius);
        void UnloadTilesOutsideRadius(const glm::vec3& centerPos, float radius);

        void GenerateGrid(int minX, int maxX, int minZ, int maxZ,
            const std::string& terrainType = "default");

        void GenerateAndLoadGrid(int minX, int maxX, int minZ, int maxZ,
            const std::string& terrainType = "default");

        Tile* GetTile(int gridX, int gridZ);
        bool HasTile(int gridX, int gridZ) const;

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

        bool GetGridBounds(glm::vec3& outMin, glm::vec3& outMax) const;
        bool GetPlayableBounds(glm::vec3& outMin, glm::vec3& outMax) const;
        void GetGridRange(int& outMinX, int& outMaxX, int& outMinZ, int& outMaxZ) const;
        // Same as GetGridRange but excludes the decorative border ring.
        void GetPlayableRange(int& outMinX, int& outMaxX, int& outMinZ, int& outMaxZ) const;


        void Clear();

        /**
         * @brief Descarca toate tile-urile fara a le sterge
         */
        void UnloadAll();
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
        std::unordered_map<int, Model3D*> m_typeModels; // TileType (as int) -> model
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

}

#endif