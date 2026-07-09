#include <iostream>
#include <cmath>
#include <string>
#include <algorithm>

#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp> 

#include "Shader.hpp"
#include "Camera.hpp"
#include "CameraIso.hpp"
#include "Model3D.hpp"
#include "SkyBox.hpp"
#include "RayCaster.hpp"
#include "TileManager.hpp"

#include "InputManager.hpp"           // Input centralizat
#include "Scene.hpp"                  // Container pentru obiecte
#include "SceneObject.hpp"            // Obiecte cu componente
#include "SceneManager.hpp"           // Manager pentru spawn și init
#include "SelectionSystem.hpp"        // Box selection
#include "CollisionSystem.hpp"        // Collision detection

#include "GuiManager.hpp"
#include "CombatSystem.hpp"
#include "ResourceManager.hpp"
#include "EditorState.hpp"
#include "Benchmark.hpp"           
#include "SelfTest.hpp"           



// WINDOW SETTINGS
int glWindowWidth = 1920;
int glWindowHeight = 1080;
int retina_width, retina_height;
GLFWwindow* glWindow = NULL;

// CAMERA & VIEW
gps::Camera myCamera(
    glm::vec3(0.0f, 200, -200)
);

float cameraSpeed = 150.0f;
float zoomFactor = 1.0f;

// Camera smoothing / interaction state (driven by processMovement).
float        targetZoomFactor   = 1.0f;
bool         cameraSeeking      = false;   // F-focus seek in progress
glm::vec3    cameraSeekTargetXZ = glm::vec3(0.0f);
bool         middleDragging     = false;
bool         dragHasPrev        = false;
glm::vec3    dragPrevWorld      = glm::vec3(0.0f);
double       lastFPressTime     = -10.0;

// Delay (seconds) before an aircraft carrier deploys a replacement plane after
// its current one is shot down.
constexpr float kAircraftRespawnDelay = 5.0f;
// Seconds of "no combat tick" at match start so the pre-staged MVG enemy
// doesn't instantly open fire while the player is still getting their bearings.
constexpr double kMatchGraceSec = 5.0;

// Single source of truth for the world Y every gameplay unit/prop/base sits on.
// This is the same ground plane screenToWorld() intersects for mouse placement,
// so anything spawned through it (bases, the pre-staged enemy force, troops,
// formations) lands coplanar with mouse-placed props instead of floating or
// sinking at mismatched heights (was 0 / -44 / -60).
constexpr float kGroundY = -60.0f;

// Forward declaration — definition lives after the global system pointers it
// uses (g_sceneManager / g_scene).
static int SpawnCarrierAircraft(gps::SceneObject* carrier);

gps::Shader myCustomShader;
gps::Shader depthMapShader;
gps::Shader skyboxShader;
gps::Shader boxSelectionShader;
gps::Shader rangeCircleShader;

// Range circle geometry
GLuint g_circleVAO = 0;
GLuint g_circleVBO = 0;
int    g_circleVertCount = 0;

// Range circle uniform locations
GLint circleModelLoc = -1;
GLint circleViewLoc  = -1;
GLint circleProjLoc  = -1;
GLint circleColorLoc = -1;

// Uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint viewPosLoc;
GLint isSelectedLoc;  // Pentru highlight

GLint highlightColorLoc;
GLint objectTintLoc;
GLint objectIDLoc;
GLint isGhostLoc;

// LIGHTING
glm::vec3 lightDir = glm::vec3(150.0f, 500.0f, 50.0f);
glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);

// Directional light
glm::vec3 dirLightWorld = glm::vec3(200.0f, 150.0f, 250.0f);
glm::vec3 dirLightColorVal = glm::vec3(0.647f, 0.565f, 0.459f);


GLuint shadowMapFBO;
GLuint depthMapTexture;
const unsigned int SHADOW_WIDTH = 4096;
const unsigned int SHADOW_HEIGHT = 4096;


GLint lightPosEyeLoc;
GLint lightColorLoc;
GLint dirLightDirEyeLoc;
GLint dirLightColorLoc;
GLint viewPosEyeLoc;

// MODELE 3D 
gps::Model3D sceneModel;      
gps::Model3D obiecte;
gps::Model3D orcModel;        
gps::Model3D rightWingModel;
gps::Model3D Pikeman;
gps::Model3D waterTyle;
gps::Model3D shipModel;
gps::Model3D oilRigModel;
gps::Model3D frigate;
gps::Model3D destroyer;
gps::Model3D islandT;
gps::Model3D aircraft;
gps::Model3D aircraftCarrier;
gps::Model3D turret;
gps::Model3D projectile;
gps::Model3D bomb;

// Hex tile models (objects/tiles/*.obj). Center-to-vertex = 5.587 in model units.
gps::Model3D waterTile;
gps::Model3D oilTile;
gps::Model3D fishTile;
gps::Model3D desertTile;
gps::Model3D greenTile;
gps::Model3D fishBoat;

// SKYBOX
gps::SkyBox mySkyBox;
std::vector<const GLchar*> faces{
    "textures/skybox/rt.tga",
    "textures/skybox/lf.tga",
    "textures/skybox/up.tga",
    "textures/skybox/dn.tga",
    "textures/skybox/bk.tga",
    "textures/skybox/ft.tga"
};

// TILE MANAGER
// Hex tile model has center-to-vertex = 5.587 in model units; tile size must equal
// modelScale * 5.587 for hexes to pack flush. Smaller tile -> denser-feeling map.
static int mapGroundY= -140;
gps::TileManager g_tileManager(100.566f, glm::vec3(18.0f), mapGroundY);


gps::Scene* g_scene = nullptr;
gps::SceneManager* g_sceneManager = nullptr;
gps::SelectionSystem* g_selectionSystem = nullptr;
gps::CollisionSystem* g_collisionSystem = nullptr;
gps::CombatSystem*   g_combatSystem    = nullptr;
gps::ResourceManager& resourceManager = gps::ResourceManager::Instance();
gps::GuiManager* g_guiManager = nullptr;
gps::EditorState* g_editorState = nullptr;
gps::BenchmarkHarness* g_benchmark = nullptr;

int g_nextPlacedPropID = 10001;

// SPARK PARTICLES (damage feedback)
struct Spark {
    glm::vec3 pos;
    glm::vec3 vel;
    float     age   = 0.0f;
    float     life  = 0.5f;
    glm::vec4 color = glm::vec4(1.0f, 0.55f, 0.15f, 1.0f);
};
std::vector<Spark> g_sparks;

// Emit a burst of sparks at `origin`. Re-used by projectile impact, mine
// detonation, and any future hit feedback that needs visual punch.
static void EmitSparks(const glm::vec3& origin, int count, const glm::vec4& color,
                       float speed = 28.0f, float life = 0.5f) {
    g_sparks.reserve(g_sparks.size() + count);
    for (int i = 0; i < count; ++i) {
        const float u = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        const float v = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        const float theta = u * 6.28318530718f;          // around the vertical axis
        const float phi   = v * 0.9f + 0.1f;             // mostly upward hemisphere
        glm::vec3 dir(std::cos(theta) * std::sin(phi),
                      std::cos(phi),
                      std::sin(theta) * std::sin(phi));
        Spark s;
        s.pos   = origin;
        s.vel   = dir * (speed * (0.6f + 0.4f * v));
        s.life  = life * (0.8f + 0.4f * u);
        s.color = color;
        g_sparks.push_back(s);
    }
}

// Apply a single damage hit to victim. Adds to killedOut on lethal hit and
// auto-assigns retaliation target (so Defensive units fight back). Shared by
// the projectile-impact path and the naval-mine detonation path.
static void ApplyDamageTo(gps::SceneObject* victim, int dmg, int attackerOwnerID,
                          std::vector<int>& killedOut) {
    if (!victim || !victim->IsActive() || !victim->unitStats.isAlive) return;
    victim->unitStats.health -= dmg;
    const bool lethal = (victim->unitStats.health <= 0);
    // Surface the hit as a floating damage number above the victim.
    if (g_guiManager) g_guiManager->PushFloatingNumber(victim->GetWorldCenter(), dmg, lethal);
    if (lethal) {
        victim->unitStats.health  = 0;
        victim->unitStats.isAlive = false;
        victim->SetActive(false);
        killedOut.push_back(victim->GetID());
    } else if (victim->unitStats.targetID == -1) {
        victim->unitStats.targetID = attackerOwnerID;
    }
}


static void CleanupKilled(const std::vector<int>& killed);

static int SpawnCarrierAircraft(gps::SceneObject* carrier) {
    if (!carrier || !g_sceneManager) return -1;
    glm::vec3 carrierPos = carrier->GetTransform().GetPosition();
    glm::vec3 aircraftPos = carrierPos + glm::vec3(carrier->unitStats.attackRange, 20.0f, 0.0f);
    gps::SceneObject* plane = g_sceneManager->SpawnObject(
        "Aircraft_for_" + std::to_string(carrier->GetID()),
        "aircraft",
        "aircraft",
        aircraftPos,
        glm::vec3(3.0f)
    );
    if (!plane) return -1;

    plane->unitStats.faction      = carrier->unitStats.faction;
    plane->orbitData.isOrbiting   = true;
    plane->orbitData.parentID     = carrier->GetID();
    plane->orbitData.orbitRadius  = carrier->unitStats.attackRange;
    plane->orbitData.orbitSpeed   = 1.0f;
    plane->orbitData.orbitAngle   = 0.0f;
    plane->orbitData.orbitHeight  = 20.0f;
    plane->SetCollisionRadius(0.0f);

    carrier->orbitData.childAircraftID = plane->GetID();
    carrier->orbitData.respawnAfter    = 0.0;
    return plane->GetID();
}

// Bases use sentinel -1 (never spawned) and -2 (was spawned, has died) so we
// can detect a fall in the same frame the SceneObject gets destroyed.
int    g_baseP1ID        = -1;
int    g_baseP2ID        = -1;
bool   g_matchActive     = false;   // true while combat should tick
bool   g_matchInitialized= false;   // true once the user has pressed Start
double g_matchStartTime  = 0.0;
int    g_matchP1Lost     = 0;
int    g_matchP2Lost     = 0;
float  g_matchOilSpent   = 0.0f;

// Bookkeep + destroy a batch of just-killed units: faction-lost counters,
// base-fall sentinels, selection cleanup, scene destroy. Same flow used by
// both projectile-splash and mine-splash paths. (Declared above; body here
// because it depends on the MATCH STATE globals.)
static void CleanupKilled(const std::vector<int>& killed) {
    for (int id : killed) {
        if (gps::SceneObject* v = g_scene->GetObjectByID(id)) {
            if      (v->unitStats.faction == 1) g_matchP1Lost++;
            else if (v->unitStats.faction == 2) g_matchP2Lost++;
        }
        if (id == g_baseP1ID) g_baseP1ID = -2;
        if (id == g_baseP2ID) g_baseP2ID = -2;
        if (g_selectionSystem) g_selectionSystem->RemoveFromSelection(id);
        g_scene->DestroyObject(id);
    }
}

double lastFrameTime = 0.0;
float deltaTime = 1.0f;

glm::mat4 projection;

// FUNCTION DECLARATIONS
void windowResizeCallback(GLFWwindow* window, int width, int height);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

bool initOpenGLWindow();
void setWindowCallbacks();
void initOpenGLState();
void initShadowFrameBuffer();
void initModels();
void initShaders();
void initUniforms();
void initSkyBox();
void initTileManager();
void initSceneManager();
void initSelectionSystem();
void initCollisionSystem();
void initCombatSystem();

void initGui();
void processMovement();
void updateDeltaTime();
void renderScene(gps::Shader shader);
void renderShadowMap();
void renderSkyBox();
void initRangeCircle();
void renderRangeCircles();
void renderSparks();
void renderDebugBounds();

glm::vec3 screenToWorld(const glm::vec2& screenPos);

// CALLBACKS IMPLEMENTATION
void windowResizeCallback(GLFWwindow* window, int width, int height) {
    glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);
    glViewport(0, 0, retina_width, retina_height);

    glWindowWidth = width;
    glWindowHeight = height;

    // Update selection system
    if (g_selectionSystem) {
        g_selectionSystem->SetScreenSize(width, height);
    }

    // Update projection
    float aspectRatio = static_cast<float>(retina_width) / static_cast<float>(retina_height);
    float orthoSize = 150.0f * zoomFactor;
    projection = glm::ortho(
        -orthoSize * aspectRatio, orthoSize * aspectRatio,
        -orthoSize, orthoSize,
        -1000.0f, 1000.0f
    );
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    // Forward către InputManager
    gps::InputManager::Instance().OnKeyEvent(key, scancode, action, mode);

    const bool textInputActive = g_guiManager && g_guiManager->WantsTextInput();
    if (!textInputActive
        && action == GLFW_PRESS
        && key >= GLFW_KEY_0 && key <= GLFW_KEY_9
        && g_selectionSystem
        && (!g_editorState || g_editorState->IsPlayMode()))
    {
        const int  slot  = key - GLFW_KEY_0;
        const bool ctrl  = (mode & GLFW_MOD_CONTROL) != 0;
        const bool shift = (mode & GLFW_MOD_SHIFT)   != 0;
        if (ctrl) {
            g_selectionSystem->SaveGroup(slot);
            std::cout << "[ControlGroup] Saved slot " << slot
                      << " (" << g_selectionSystem->GetGroupSize(slot) << " units)" << std::endl;
        } else {
            g_selectionSystem->RecallGroup(slot, /*append=*/shift);
            std::cout << "[ControlGroup] Recalled slot " << slot
                      << (shift ? " (append)" : "") << std::endl;
        }
        return;
    }

    if (g_guiManager && g_guiManager->WantsKeyboardInput()) {
        return;
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    gps::InputManager::Instance().OnMouseButton(button, action, mods);

    // Middle-mouse drag-to-pan camera. Always tracked (even over ImGui release),
    // so a release that lands over a panel still ends the drag cleanly.
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS && !(g_guiManager && g_guiManager->WantsMouseInput())) {
            middleDragging = true;
            dragHasPrev    = false;
        } else if (action == GLFW_RELEASE) {
            middleDragging = false;
            dragHasPrev    = false;
        }
        return;
    }

    if (g_guiManager && g_guiManager->WantsMouseInput()) {
        return;
    }

    if (!g_selectionSystem || !g_scene) return;

    glm::vec2 mousePos = gps::InputManager::Instance().GetMousePosition();

    //  EDIT MODE INPUT
    if (g_editorState && g_editorState->IsEditMode()) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                int hitID = g_selectionSystem->GetObjectAtPoint(*g_scene, myCamera, projection, mousePos);

                if (hitID >= 0 && g_selectionSystem->IsSelected(hitID)) {
                    // Start dragging the already-selected object
                    gps::SceneObject* obj = g_scene->GetObjectByID(hitID);
                    if (obj) {
                        glm::vec3 worldPos = screenToWorld(mousePos);
                        g_editorState->StartDrag(hitID, obj->GetTransform().GetPosition(), worldPos);
                    }
                } else {
                    // Not on a selected object -> do normal selection
                    g_selectionSystem->StartBoxSelection(mousePos);
                }
            }
            else if (action == GLFW_RELEASE) {
                if (g_editorState->IsDragging()) {
                    g_editorState->EndDrag();
                } else {
                    bool shiftHeld = gps::InputManager::Instance().IsKeyPressed(GLFW_KEY_LEFT_SHIFT) ||
                        gps::InputManager::Instance().IsKeyPressed(GLFW_KEY_RIGHT_SHIFT);
                    g_selectionSystem->EndBoxSelection(*g_scene, myCamera, projection, shiftHeld);
                }
            }

        }
        // In edit mode, right-click does nothing (no movement commands)
        return;
    }

    if (g_sceneManager->m_patrolTargeting && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        glm::vec3 clicked = screenToWorld(mousePos);

        // Clamp to the playable grid AABB so we can't drop a waypoint into the void.
        glm::vec3 gridMin, gridMax;
        if (g_tileManager.GetPlayableBounds(gridMin, gridMax)) {
            clicked.x = glm::clamp(clicked.x, gridMin.x, gridMax.x);
            clicked.z = glm::clamp(clicked.z, gridMin.z, gridMax.z);
        }

        // Reject Land / border / void — ships can't reach them and the arrival
        // hook would silently halt the patrol there. Same rule as the movement
        // path in the main loop.
        int gx, gz;
        g_tileManager.WorldToGrid(clicked, gx, gz);
        gps::Tile* t = g_tileManager.GetTile(gx, gz);
        if (!t || t->isBorder || t->type == gps::TileType::Land) {
            std::cout << "Patrol waypoint rejected (not navigable)" << std::endl;
            return;
        }

        if (g_sceneManager->m_patrolClickPhase == 0) {
            g_sceneManager->m_patrolPointA   = clicked;
            g_sceneManager->m_patrolClickPhase = 1;
        } else {
            const glm::vec3 A = g_sceneManager->m_patrolPointA;
            const glm::vec3 B = clicked;

            // Centroid of the snapshotted movable units, so the patrol formation
            // preserves the shape the group has right now.
            glm::vec3 centroid(0.0f);
            int n = 0;
            for (int id : g_sceneManager->m_patrolUnitIDs) {
                gps::SceneObject* obj = g_scene->GetObjectByID(id);
                if (!obj || !obj->unitStats.isMovable) continue;
                centroid += obj->GetTransform().GetPosition();
                ++n;
            }
            if (n > 0) centroid /= static_cast<float>(n);

            for (int id : g_sceneManager->m_patrolUnitIDs) {
                gps::SceneObject* obj = g_scene->GetObjectByID(id);
                if (!obj || !obj->unitStats.isMovable) continue;

                glm::vec3 offset = obj->GetTransform().GetPosition() - centroid;
                offset.y = 0.0f;

                obj->patrolData.isPatrolling       = true;
                obj->patrolData.pointA             = A;
                obj->patrolData.pointB             = B;
                obj->patrolData.offsetFromCentroid = offset;
                obj->patrolData.headingToB         = true;

                glm::vec3 target = B + offset;
                if (g_tileManager.GetPlayableBounds(gridMin, gridMax)) {
                    target.x = glm::clamp(target.x, gridMin.x, gridMax.x);
                    target.z = glm::clamp(target.z, gridMin.z, gridMax.z);
                }
                glm::vec3 dir = target - obj->GetTransform().GetPosition();
                dir.y = 0.0f;
                if (glm::length(dir) > 0.0001f) obj->movement.moveDirection = glm::normalize(dir);

                obj->movement.isMoving      = true;
                obj->movement.moveStartPos  = obj->GetTransform().GetPosition();
                obj->movement.moveEndPos    = target;
                obj->movement.moveStartTime = glfwGetTime();
                obj->movement.moveDuration  = 2.0f;
                int dgx, dgz;
                g_tileManager.WorldToGrid(target, dgx, dgz);
                if (gps::Tile* dt = g_tileManager.GetTile(dgx, dgz))
                    obj->movement.moveDuration *= gps::GetTileEffect(dt->type).moveDurationMul;
            }
            // Exit targeting mode.
            g_sceneManager->m_patrolTargeting  = false;
            g_sceneManager->m_patrolClickPhase = 0;
            g_sceneManager->m_patrolUnitIDs.clear();
        }
        return;
    }

    if (g_sceneManager->m_bombardmentTargeting && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        g_sceneManager->m_bombardmentTargeting = false;
        if (resourceManager.Spend("Oil", 75.0f)) {
            g_matchOilSpent += 75.0f;
            glm::vec3 target = screenToWorld(mousePos);
            if (g_combatSystem) g_combatSystem->SpawnBombardment(target, 1 /* player faction */);
        }
        return;
    }

    // Placement mode: left click places selected prop and skips selection box logic.
    if (g_sceneManager->m_propPlacementMode && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        if (g_sceneManager) {
            glm::vec3 worldPos = screenToWorld(mousePos);

            glm::vec3 gridMin, gridMax;
            if (g_tileManager.GetPlayableBounds(gridMin, gridMax)) {
                worldPos.x = glm::clamp(worldPos.x, gridMin.x, gridMax.x);
                worldPos.z = glm::clamp(worldPos.z, gridMin.z, gridMax.z);
            }

            std::string objectName = "Prop_" + g_sceneManager->m_propPlacementTag + "_" + std::to_string(g_nextPlacedPropID++);
            gps::SceneObject* spawned = g_sceneManager->SpawnObject(
                objectName,
				g_sceneManager->m_propPlacementTag,
                g_sceneManager->m_propPlacementModelName,
                worldPos,
                g_sceneManager->m_propPlacementScale
            );

            // Reject the placement if the new object overlaps anything collidable.
            if (spawned && g_collisionSystem && g_collisionSystem->GetCollidingObject(spawned)) {
                std::cout << "Placement blocked: overlaps another object" << std::endl;
                g_scene->DestroyObject(spawned->GetID());
                spawned = nullptr;
                return;
            }

            int gx, gz;
            g_tileManager.WorldToGrid(spawned->GetTransform().GetPosition(), gx, gz);
            bool matchingTile = false;

            if (gps::Tile* t = g_tileManager.GetTile(gx, gz)) {
                // Block placement on the decorative border ring.
                if (t->isBorder) {
                    g_scene->DestroyObject(spawned->GetID());
                    spawned = nullptr;
                    return;
                }
                if(t->type != gps::TileType::Land && spawned->GetTag() == "turret") {
                        g_scene->DestroyObject(spawned->GetID());
                        spawned = nullptr;
                         return;
                }
                // Mines must sit on water (Sea / Shallows / Oil / Fish) — they
                // can't be placed on Land since the ships they're meant to
                // catch never go there.
                if (spawned->GetTag() == "mine" && t->type == gps::TileType::Land) {
                    g_scene->DestroyObject(spawned->GetID());
                    spawned = nullptr;
                    return;
                }
            } else {
                // Click landed outside any tile (shouldn't happen after clamp,
                // but reject defensively rather than spawn into the void).
                g_scene->DestroyObject(spawned->GetID());
                spawned = nullptr;
                return;
            }
            
            if (spawned) {
                const gps::UnitStats::PricePoints& price = spawned->unitStats.price;
                if (!resourceManager.Spend({ {"Oil",  (float)price.oil},
                                             {"Fish", (float)price.fish} })) {
                    std::cout << "Placement blocked: not enough resources ("
                              << price.oil << " Oil / " << price.fish << " Fish)" << std::endl;
                    g_scene->DestroyObject(spawned->GetID());
                    spawned = nullptr;
                    return;
                }
                g_matchOilSpent += price.oil;

                // Apply the selected faction (Friendly / Enemy) over whatever
                // InitializeUnitsStats defaulted to based on the unit's tag.
                spawned->unitStats.faction = g_sceneManager->m_propPlacementFaction;

                std::cout << "Placed " << g_sceneManager->m_propPlacementLabel << " at ("
                    << worldPos.x << ", " << worldPos.y << ", " << worldPos.z << ")" << std::endl;

                if (spawned->GetTag() == "aircraftCarrier") {
                    SpawnCarrierAircraft(spawned);
                }
            }
        }
        return;
    }

    //Selection
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {

			int hitID = g_selectionSystem->GetObjectAtPoint(*g_scene, myCamera, projection, mousePos);
            gps::SceneObject* prop = g_scene->GetObjectByID(hitID);

            if (hitID != -1 && g_selectionSystem->HasSelection() && !g_selectionSystem->IsSelected(hitID)) {
                gps::SceneObject* target = g_scene->GetObjectByID(hitID);
                if (target && target->unitStats.isAlive)
                {
                    int atkFaction = 0;
                    const auto& selIDs = g_selectionSystem->GetSelectedIDs();
                    if (!selIDs.empty()) {
                        const int firstSelectedID = *selIDs.begin();
                        gps::SceneObject* first = g_scene->GetObjectByID(firstSelectedID);
                        if (first) atkFaction = first->unitStats.faction;
                    }
                    int tgtFaction = target->unitStats.faction;
                    bool sameFaction = (atkFaction != 0 && tgtFaction != 0 && atkFaction == tgtFaction);
                    if (!sameFaction) {
                        for (int selfID : selIDs) {
                            gps::SceneObject* attacker = g_scene->GetObjectByID(selfID);
                            if (attacker) attacker->unitStats.targetID = hitID;
                        }
                        return;
                    }
                }
                if (prop && prop->GetTag() == "tile")
                {
                    g_selectionSystem->ClearSelection();
                    return;
                }
            }

            g_selectionSystem->StartBoxSelection(mousePos);
        }
        else if (action == GLFW_RELEASE) {
            bool shiftHeld = gps::InputManager::Instance().IsKeyPressed(GLFW_KEY_LEFT_SHIFT) ||
                gps::InputManager::Instance().IsKeyPressed(GLFW_KEY_RIGHT_SHIFT);

            g_selectionSystem->EndBoxSelection(*g_scene, myCamera, projection, shiftHeld);
        }
    }

    // Cancel placement mode
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS && g_sceneManager->m_propPlacementMode) {
        g_sceneManager->CancelPropPlacement();
        return;
    }

    //Movement Command
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        glm::vec3 worldPos = screenToWorld(mousePos);

        const auto& selectedIDs = g_selectionSystem->GetSelectedIDs();
        if (selectedIDs.empty()) return;

        glm::vec3 gridMin, gridMax;
        bool hasGrid = g_tileManager.GetPlayableBounds(gridMin, gridMax);

        // Preserve current formation: compute the centroid of all movable units
        // and translate that centroid onto the click point. Each unit then moves
        // to (worldPos + its own offset from centroid), so the group keeps its
        // relative shape instead of stacking up on the same destination.
        glm::vec3 centroid(0.0f);
        int movableCount = 0;
        for (int troopID : selectedIDs) {
            gps::SceneObject* obj = g_scene->GetObjectByID(troopID);
            if (!obj || !obj->unitStats.isMovable) continue;
            centroid += obj->GetTransform().GetPosition();
            ++movableCount;
        }
        if (movableCount == 0) return;
        centroid /= static_cast<float>(movableCount);

        for (int troopID : selectedIDs) {
            gps::SceneObject* obj = g_scene->GetObjectByID(troopID);
            if (!obj) continue;
            if (!obj->unitStats.isMovable) continue;

            glm::vec3 startPos = obj->GetTransform().GetPosition();
            glm::vec3 offset = startPos - centroid;
            offset.y = 0.0f;
            glm::vec3 formationPos = worldPos + offset;

            if (hasGrid) {
                formationPos.x = glm::clamp(formationPos.x, gridMin.x, gridMax.x);
                formationPos.z = glm::clamp(formationPos.z, gridMin.z, gridMax.z);
            }

            glm::vec3 moveDir = formationPos - startPos;
            moveDir.y = 0.0f;

            if (glm::length(moveDir) > 0.0001f) {
                moveDir = glm::normalize(moveDir);
                obj->movement.moveDirection = moveDir;
            }

            obj->movement.isMoving = true;
            obj->movement.moveStartPos = startPos;
            obj->movement.moveEndPos = formationPos;
            obj->movement.moveStartTime = glfwGetTime();
            obj->movement.moveDuration = 2.0f;

            // Slow-tile gimmick: stretch duration based on the destination tile.
            int dgx, dgz;
            g_tileManager.WorldToGrid(formationPos, dgx, dgz);
            if (gps::Tile* destTile = g_tileManager.GetTile(dgx, dgz)) {
                obj->movement.moveDuration *= gps::GetTileEffect(destTile->type).moveDurationMul;
            }

            // Manual move order always cancels an active patrol on this unit.
            obj->patrolData.isPatrolling = false;
        }
    }
}

void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
    gps::InputManager::Instance().OnMouseMove(xpos, ypos);

    // Edit mode drag
    if (g_editorState && g_editorState->IsEditMode() && g_editorState->IsDragging()) {
        glm::vec3 currentWorldPos = screenToWorld(glm::vec2(xpos, ypos));

        int draggedID = g_editorState->GetDraggedObjectID();
        gps::SceneObject* obj = g_scene->GetObjectByID(draggedID);
        if (obj) {
            glm::vec3 offset = g_editorState->GetDragOffset();
            glm::vec3 newPos = currentWorldPos + offset;
            newPos.y = obj->GetTransform().GetPosition().y; // Keep Y unchanged
            obj->GetTransform().SetPosition(newPos);
            obj->UpdateWorldBounds();
        }
        return;
    }

    // Update box selection dacă e activă
    if (g_selectionSystem && g_selectionSystem->IsBoxSelecting()) {
        g_selectionSystem->UpdateBoxSelection(glm::vec2(xpos, ypos));
    }
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    gps::InputManager::Instance().OnMouseScroll(xoffset, yoffset);

    // Don't react to scroll while a panel is hovered/focused.
    if (g_guiManager && g_guiManager->WantsMouseInput()) return;

    // Push a new target; the actual zoomFactor eases toward it in processMovement.
    targetZoomFactor *= (1.0f - static_cast<float>(yoffset) * 0.1f);

    // Derive max zoom from the grid AABB so the whole map (incl. border ring)
    // can fit on-screen even on smaller resolutions. 0.707 accounts for the
    // ~45 deg camera tilt that compresses world-Z onto the screen-Y axis.
    float maxZ = 2.0f;
    glm::vec3 gMin, gMax;
    if (g_tileManager.GetGridBounds(gMin, gMax)) {
        const float aspect = static_cast<float>(retina_width) / static_cast<float>(retina_height);
        const float W = gMax.x - gMin.x;
        const float H = (gMax.z - gMin.z) * 0.707f;
        const float needOrtho = std::max(W / (2.0f * aspect), H * 0.5f) * 1.05f;
        maxZ = std::max(2.0f, needOrtho / 150.0f);
    }
    targetZoomFactor = glm::clamp(targetZoomFactor, 0.5f, maxZ);
}

// GUI INIT
void initGui() {
    g_guiManager = new gps::GuiManager();
    g_guiManager->Initialize(glWindow, "#version 410");
    g_guiManager->BindSystems(g_scene, g_sceneManager, g_selectionSystem, &g_tileManager);
    std::cout << "GUI initialized" << std::endl;
}

// INITIALIZATION
bool initOpenGLWindow() {
    if (!glfwInit()) {
        std::cerr << "ERROR: could not start GLFW3" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "OpenGL Project", NULL, NULL);
    if (!glWindow) {
        std::cerr << "ERROR: could not open window with GLFW3" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(glWindow);
    glfwSwapInterval(1);

#if !defined (__APPLE__)
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "ERROR: GLEW init failed" << std::endl;
        return false;
    }
#endif

    glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

    return true;
}

void setWindowCallbacks() {
    glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
    glfwSetKeyCallback(glWindow, keyCallback);
    glfwSetMouseButtonCallback(glWindow, mouseButtonCallback);
    glfwSetCursorPosCallback(glWindow, cursorPositionCallback);
    glfwSetScrollCallback(glWindow, scrollCallback);
}

void initOpenGLState() {
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glViewport(0, 0, retina_width, retina_height);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_FRAMEBUFFER_SRGB);
}

void initShadowFrameBuffer() {
    glGenFramebuffers(1, &shadowMapFBO);

    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initModels() {
    std::cout << "Loading models" << std::endl;

    sceneModel.LoadModel("objects/teren/teren.obj", "textures/");
    obiecte.LoadModel("objects/restobiecte/restobiecte.obj", "textures/");
    orcModel.LoadModel("objects/ORC/ORC.obj", "textures/");
	Pikeman.LoadModel("objects/ORC/pikeman.obj", "textures/");
    rightWingModel.LoadModel("objects/RIGHTWING/RIGHTWING.obj", "textures/");
    waterTyle.LoadModel("objects/water/water.obj","textures/");
    shipModel.LoadModel("objects/ships/Battleship.obj","textures/");
    oilRigModel.LoadModel("objects/oilRig/oilRig.obj", "textures/");
	frigate.LoadModel("objects/frigate/Frigate.obj", "textures/");
    destroyer.LoadModel("objects/destroyer/Destroyer.obj", "textures/");
    islandT.LoadModel("objects/islandT/islandT.obj","textures/");
    aircraft.LoadModel("objects/aircraft/aircraft.obj","textures/");
    aircraftCarrier.LoadModel("objects/aircraftCarrier/AircraftCarrier.obj","textures/");
    turret.LoadModel("objects/tiles/ciws.obj", "textures/");
    projectile.LoadModel("objects/projectile/Projectile.obj", "textures/");
    bomb.LoadModel("objects/bomb/bomb.obj", "textures/");
    waterTile.LoadModel("objects/tiles/waterTile.obj", "textures/");
    oilTile.LoadModel("objects/tiles/oilTile.obj", "textures/");
    fishTile.LoadModel("objects/tiles/fishTile.obj", "textures/");
    desertTile.LoadModel("objects/tiles/desertTile.obj", "textures/");
    greenTile.LoadModel("objects/tiles/greenTile.obj", "textures/");
    fishBoat.LoadModel("objects/tiles/fishBoat.obj", "textures/");

    std::cout << "Models loaded" << std::endl;
}

void initShaders() {
    myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
    myCustomShader.useShaderProgram();

    depthMapShader.loadShader("shaders/depth.vert", "shaders/depth.frag");
    depthMapShader.useShaderProgram();

    skyboxShader.loadShader("shaders/skybox.vert", "shaders/skybox.frag");
    skyboxShader.useShaderProgram();

}

void initUniforms() {
    myCustomShader.useShaderProgram();

    modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
    viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
    projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
    normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
    lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");
    viewPosLoc = glGetUniformLocation(myCustomShader.shaderProgram, "viewPos");
    isSelectedLoc = glGetUniformLocation(myCustomShader.shaderProgram, "isSelected");
    highlightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "highlightColor");
    objectTintLoc     = glGetUniformLocation(myCustomShader.shaderProgram, "objectTint");
    objectIDLoc = glGetUniformLocation(myCustomShader.shaderProgram, "objectID");
    isGhostLoc = glGetUniformLocation(myCustomShader.shaderProgram, "isGhost");

  // Set light direction
  //  glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

    // Set view
    glm::mat4 view = myCamera.getViewMatrix();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // Set projection
    float aspectRatio = static_cast<float>(retina_width) / static_cast<float>(retina_height);
    float orthoSize = 150.0f * zoomFactor;
    projection = glm::ortho(
        -orthoSize * aspectRatio, orthoSize * aspectRatio,
        -orthoSize, orthoSize,
        -1000.0f, 1000.0f
    );
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));


    lightPosEyeLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightPosEye");
    glm::vec3 pointLightPosEye = glm::vec3(view * glm::vec4(lightDir, 1.0f));
    glUniform3fv(lightPosEyeLoc, 1, glm::value_ptr(pointLightPosEye));

    // Point light color
    lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    // Directional light în EYE space
    dirLightDirEyeLoc = glGetUniformLocation(myCustomShader.shaderProgram, "dirLightDirEye");
    dirLightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "dirLightColor");
    glm::vec3 dirLightEye = glm::inverseTranspose(glm::mat3(view)) * dirLightWorld;
    glUniform3fv(dirLightDirEyeLoc, 1, glm::value_ptr(dirLightEye));
    glUniform3fv(dirLightColorLoc, 1, glm::value_ptr(dirLightColorVal));

    // Camera position în EYE space
    glm::mat4 invView = glm::inverse(view);
    glm::vec3 cameraPosWorld = glm::vec3(invView[3]);
    glm::vec3 cameraPosEye = glm::vec3(view * glm::vec4(cameraPosWorld, 1.0f));
    viewPosEyeLoc = glGetUniformLocation(myCustomShader.shaderProgram, "viewPosEye");
    glUniform3fv(viewPosEyeLoc, 1, glm::value_ptr(cameraPosEye));

}

void initSkyBox() {
    mySkyBox.Load(faces);
}

void initSceneManager() {
    // 1. Creeaza scena
    g_scene = new gps::Scene("MainScene");

    // 2. Creeaza scene manager
    g_sceneManager = new gps::SceneManager();
    g_sceneManager->Initialize(g_scene, &g_tileManager);

    // 3. Inregistreaza modelele
    g_sceneManager->RegisterModel("terrain", &sceneModel);
    g_sceneManager->RegisterModel("objects", &obiecte);
    g_sceneManager->RegisterModel("orc", &orcModel);
	g_sceneManager->RegisterModel("pikeman", &Pikeman);
    g_sceneManager->RegisterModel("water",&waterTyle);
    g_sceneManager->RegisterModel("ship",&shipModel);
    g_sceneManager->RegisterModel("oilRig", &oilRigModel);
	g_sceneManager->RegisterModel("frigate", &frigate);
	g_sceneManager->RegisterModel("destroyer", &destroyer); 
    g_sceneManager->RegisterModel("islandT", &islandT);
    g_sceneManager->RegisterModel("aircraft", &aircraft);
    g_sceneManager->RegisterModel("aircraftCarrier",&aircraftCarrier);
    g_sceneManager->RegisterModel("turret", &turret);
    g_sceneManager->RegisterModel("projectile", &projectile);
    g_sceneManager->RegisterModel("bomb", &bomb);
    g_sceneManager->RegisterModel("fishBoat", &fishBoat);

    g_tileManager.Initialize(&waterTile, g_scene);
    g_tileManager.RegisterTileModel(gps::TileType::Sea,      &waterTile);
    g_tileManager.RegisterTileModel(gps::TileType::Oil,      &oilTile);
    g_tileManager.RegisterTileModel(gps::TileType::Fish,     &fishTile);
    g_tileManager.RegisterTileModel(gps::TileType::Shallows, &desertTile);
    g_tileManager.RegisterTileModel(gps::TileType::Land,     &greenTile);
    // Generate playable grid + a half-scale border ring around it.
    // Larger grid gives the cluster-based procedural gen room to form visible
    // islands; the scroll-wheel max zoom auto-derives from these bounds.
    g_tileManager.GenerateAndLoadGrid(-30, 10, -10, 10);

    // 4. Setup scena
    g_sceneManager->SetupScene();

    // 5. Enable spawn
    g_sceneManager->SetSpawnEnabled(true);

    std::cout << "SceneManager initialized with "
        << g_scene->GetObjectCount() << " objects" << std::endl;
}

// Stamps a guaranteed island at each of two opposite playable corners and
// spawns a giant oil-rig "base" on top. Faction baked into the tag via
// SceneManager::InitializeUnitsStats ("baseP1" -> faction 1, etc.).
void spawnBases() {
    int minX, maxX, minZ, maxZ;
    g_tileManager.GetPlayableRange(minX, maxX, minZ, maxZ);

    // Inset by 1 so the island ring stays inside the playable area.
    const int c1x = minX + 1, c1z = minZ + 1;
    const int c2x = maxX - 1, c2z = maxZ - 1;
    g_tileManager.StampIslandAt(c1x, c1z);
    g_tileManager.StampIslandAt(c2x, c2z);

    glm::vec3 p1 = g_tileManager.GridToWorld(c1x, c1z);
    glm::vec3 p2 = g_tileManager.GridToWorld(c2x, c2z);
    p1.y = kGroundY; p2.y = kGroundY;
    const glm::vec3 baseScale(10.0f);

    gps::SceneObject* b1 = g_sceneManager->SpawnObject("Base_P1", "baseP1", "oilRig", p1, baseScale);
    gps::SceneObject* b2 = g_sceneManager->SpawnObject("Base_P2", "baseP2", "oilRig", p2, baseScale);
    g_baseP1ID = b1 ? b1->GetID() : -1;
    g_baseP2ID = b2 ? b2->GetID() : -1;
}

// MVG seed: at match start the enemy (faction 2) corner already has economy,
// defense, and an active fleet so the demo shows a real fight from frame 1.
static gps::SceneObject* SpawnEnemyUnit(const std::string& name, const std::string& tag,
                                       const std::string& model, const glm::vec3& pos, float scale) {
    gps::SceneObject* obj = g_sceneManager->SpawnObject(name, tag, model, pos, glm::vec3(scale));
    if (obj) obj->unitStats.faction = 2;
    return obj;
}

void SpawnEnemyStartingForce() {
    if (!g_sceneManager || !g_scene) return;

    int minX, maxX, minZ, maxZ;
    g_tileManager.GetPlayableRange(minX, maxX, minZ, maxZ);
    const int cx = maxX - 1, cz = maxZ - 1;          // P2 corner (matches spawnBases)

    // Match the ground plane used by mouse placement (screenToWorld returns
    // y=kGroundY), so enemy units sit on the same surface as user-placed ones.
    auto tileAt = [&](int gx, int gz) {
        glm::vec3 p = g_tileManager.GridToWorld(gx, gz);
        p.y = kGroundY;
        return p;
    };

    // Pin every spawn cell to the tile type that unit actually needs, so the
    // procedural generator's random output can't strand a ship on Land or
    // float a turret over open water.
    auto force = [&](int gx, int gz, gps::TileType t) {
        if (gps::Tile* tile = g_tileManager.GetTile(gx, gz))
            if (!tile->isBorder && tile->type != t)
                g_tileManager.SetTileType(gx, gz, t);
    };

    // Stamp two extra land tiles a few hexes inland from the base so the
    // turrets have a Land tile to sit on, plus force the turret cells to Land
    // explicitly (StampIslandAt rings them with Shallows but we want a clean
    // platform under each turret).
    g_tileManager.StampIslandAt(cx - 2, cz);
    g_tileManager.StampIslandAt(cx, cz - 2);
    force(cx - 2, cz, gps::TileType::Land);
    force(cx, cz - 2, gps::TileType::Land);

    // Oil rigs sit in shallow water adjacent to the base island.
    force(cx + 1, cz, gps::TileType::Shallows);
    force(cx, cz + 1, gps::TileType::Shallows);
    SpawnEnemyUnit("Enemy_OilRig_1", "oilRig", "oilRig", tileAt(cx + 1, cz), 2.5f);
    SpawnEnemyUnit("Enemy_OilRig_2", "oilRig", "oilRig", tileAt(cx, cz + 1), 2.5f);

    // Defense — CIWS on the freshly-stamped Land tiles.
    SpawnEnemyUnit("Enemy_Turret_1", "turret", "turret", tileAt(cx - 2, cz), 15.0f);
    SpawnEnemyUnit("Enemy_Turret_2", "turret", "turret", tileAt(cx, cz - 2), 15.0f);

    // Army — ships + frigate + carrier in open water. Force each cell to Sea
    // so the procedural Land/Shallows assignment doesn't ground them.
    const std::pair<int,int> navalCells[] = {
        {cx - 3, cz - 1}, {cx - 1, cz - 3}, {cx - 3, cz - 3}, {cx - 4, cz - 4}
    };
    for (const auto& [gx, gz] : navalCells) force(gx, gz, gps::TileType::Sea);

    SpawnEnemyUnit("Enemy_Ship_1", "ship",    "ship",    tileAt(cx - 3, cz - 1), 6.5f);
    SpawnEnemyUnit("Enemy_Ship_2", "ship",    "ship",    tileAt(cx - 1, cz - 3), 6.5f);
    SpawnEnemyUnit("Enemy_Frigate","frigate", "frigate", tileAt(cx - 3, cz - 3), 6.5f);
    gps::SceneObject* carrier = SpawnEnemyUnit("Enemy_Carrier", "aircraftCarrier",
                                               "aircraftCarrier",
                                               tileAt(cx - 4, cz - 4),
                                               6.5f);
    if (carrier) SpawnCarrierAircraft(carrier);
}

// Tears down every non-tile object, regenerates the tile map with a fresh
// random layout, resets match counters, and re-spawns the bases on the new
// map. The Start overlay reappears via GuiManager's m_gameStarted = false.
void resetMatch() {
    if (g_scene) {
        std::vector<int> toKill;
        for (const auto& o : g_scene->GetObjects()) {
            gps::SceneObject* obj = o.get();
            if (!obj) continue;
            const std::string& tag = obj->GetTag();
            if (tag == "tile" || tag == "tileBorder") continue;
            toKill.push_back(obj->GetID());
        }
        for (int id : toKill) g_scene->DestroyObject(id);
    }
    if (g_selectionSystem) g_selectionSystem->ClearSelection();

    // Wipe the old map and roll a fresh random layout so every match feels new.
    g_tileManager.Clear();
    g_tileManager.GenerateAndLoadGrid(-30, 10, -10, 10);

    g_matchActive      = false;
    g_matchInitialized = false;
    g_matchP1Lost      = 0;
    g_matchP2Lost      = 0;
    g_matchOilSpent    = 0.0f;
    g_baseP1ID = g_baseP2ID = -1;

    gps::ResourceManager::Instance().Set("Oil",  50.0f);
    gps::ResourceManager::Instance().Set("Fish",  0.0f);

    spawnBases();
    SpawnEnemyStartingForce();
}

void initSelectionSystem() {
    g_selectionSystem = new gps::SelectionSystem();
    g_selectionSystem->Initialize(glWindowWidth, glWindowHeight);
    g_selectionSystem->LoadShader("shaders/boxSelection.vert", "shaders/boxSelection.frag");
    g_selectionSystem->SetBoxColor(glm::vec4(0.3f, 0.6f, 1.0f, 0.3f));

    std::cout << "✅ SelectionSystem initialized" << std::endl;
}

void initCollisionSystem() {
    g_collisionSystem = new gps::CollisionSystem();
    g_collisionSystem->Initialize(g_scene);
    std::cout << "✅ CollisionSystem initialized" << std::endl;
}

void initCombatSystem() {
    g_combatSystem = new gps::CombatSystem();
    g_combatSystem->Initialize(g_scene, g_sceneManager);

    // Starting resources
    gps::ResourceManager::Instance().Set("Oil",  50.0f);

    std::cout << " CombatSystem initialized" << std::endl;
}

// RANGE CIRCLE
void initRangeCircle() {
    const int N = 64;
    std::vector<glm::vec3> verts;
    verts.reserve(N);
    for (int i = 0; i < N; i++) {
        float angle = 2.0f * glm::pi<float>() * i / N;
        verts.push_back({ std::cos(angle), 0.0f, std::sin(angle) });
    }
    g_circleVertCount = N;

    glGenVertexArrays(1, &g_circleVAO);
    glGenBuffers(1, &g_circleVBO);
    glBindVertexArray(g_circleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_circleVBO);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(verts.size() * sizeof(glm::vec3)), verts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glBindVertexArray(0);

    rangeCircleShader.loadShader("shaders/rangeCircle.vert", "shaders/rangeCircle.frag");
    circleModelLoc = glGetUniformLocation(rangeCircleShader.shaderProgram, "model");
    circleViewLoc  = glGetUniformLocation(rangeCircleShader.shaderProgram, "view");
    circleProjLoc  = glGetUniformLocation(rangeCircleShader.shaderProgram, "projection");
    circleColorLoc = glGetUniformLocation(rangeCircleShader.shaderProgram, "circleColor");

    std::cout << "Range circle initialized" << std::endl;
}

void renderRangeCircles() {
    if (!g_scene || !g_selectionSystem || g_circleVAO == 0) return;

    rangeCircleShader.useShaderProgram();

    glm::mat4 view = myCamera.getViewMatrix();
    glUniformMatrix4fv(circleViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(circleProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2.0f);

    glBindVertexArray(g_circleVAO);

    if (g_sceneManager && g_sceneManager->m_bombardmentTargeting) {
        glm::vec3 pos = screenToWorld(gps::InputManager::Instance().GetMousePosition());
        pos.y += 2.0f; 
        float aoeRadius = 40.0f;
        glm::mat4 model = glm::scale(
            glm::translate(glm::mat4(1.0f), pos),
            glm::vec3(aoeRadius, 1.0f, aoeRadius)
        );
        glUniformMatrix4fv(circleModelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glm::vec4 color = glm::vec4(1.0f, 0.5f, 0.0f, 0.8f); // orange
        glUniform4fv(circleColorLoc, 1, glm::value_ptr(color));
        glDrawArrays(GL_LINE_LOOP, 0, g_circleVertCount);
    }


    // Helper: draws the attack-range ring for a combat unit at its current pos.
    // Pulled out so we can also surface the carrier's deployed plane range when
    // the carrier (not the plane) is what the player has selected.
    auto drawUnitRange = [&](gps::SceneObject* obj, const glm::vec4& colorOverride, bool useOverride) {
        if (!obj || !obj->IsActive()) return;
        if (!obj->unitStats.isCombatUnit || obj->unitStats.attackRange <= 0.0f) return;
        glm::vec3 pos = obj->GetTransform().GetPosition();
        pos.y += 2.0f;
        float range = obj->unitStats.attackRange;
        glm::mat4 model = glm::scale(
            glm::translate(glm::mat4(1.0f), pos),
            glm::vec3(range, 1.0f, range));
        glUniformMatrix4fv(circleModelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glm::vec4 color;
        if (useOverride) {
            color = colorOverride;
        } else {
            switch (obj->unitStats.faction) {
                case 1:  color = glm::vec4(0.0f, 1.0f, 0.2f, 0.9f); break;
                case 2:  color = glm::vec4(1.0f, 0.2f, 0.0f, 0.9f); break;
                default: color = glm::vec4(1.0f, 1.0f, 1.0f, 0.9f); break;
            }
        }
        glUniform4fv(circleColorLoc, 1, glm::value_ptr(color));
        glDrawArrays(GL_LINE_LOOP, 0, g_circleVertCount);
    };

    for (int id : g_selectionSystem->GetSelectedIDs()) {
        gps::SceneObject* obj = g_scene->GetObjectByID(id);
        if (!obj) continue;
        drawUnitRange(obj, glm::vec4(0.0f), false);

        // When a carrier is selected, also surface the deployed plane's range
        // (dashed-cyan-ish tint) so the player can read the plane's threat zone.
        if (obj->GetTag() == "aircraftCarrier" && obj->orbitData.childAircraftID >= 0) {
            gps::SceneObject* plane = g_scene->GetObjectByID(obj->orbitData.childAircraftID);
            drawUnitRange(plane, glm::vec4(0.4f, 0.85f, 1.0f, 0.9f), true);
        }
    }

    glBindVertexArray(0);
    glLineWidth(1.0f);
    glDisable(GL_BLEND);
}

// Per-frame: integrate spark velocity + age, drop dead ones. Then draw each as
// a tiny ring using the existing range-circle shader/VAO so we don't need a new
// shader just for this. Lifetime ~0.5s, gravity pulls them down for a brief arc.
void renderSparks() {
    if (g_circleVAO == 0) return;

    // Integrate first (separate from rendering so the same code path advances
    // sparks during a frame where the camera might not have updated yet).
    constexpr float kGravity = 60.0f;
    for (size_t i = 0; i < g_sparks.size();) {
        Spark& s = g_sparks[i];
        s.age += deltaTime;
        if (s.age >= s.life) {
            g_sparks[i] = g_sparks.back();
            g_sparks.pop_back();
            continue;
        }
        s.vel.y -= kGravity * deltaTime;
        s.pos   += s.vel   * deltaTime;
        ++i;
    }
    if (g_sparks.empty()) return;

    rangeCircleShader.useShaderProgram();
    glm::mat4 view = myCamera.getViewMatrix();
    glUniformMatrix4fv(circleViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(circleProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2.0f);
    glBindVertexArray(g_circleVAO);

    for (const Spark& s : g_sparks) {
        const float t      = s.age / s.life;
        const float alpha  = (1.0f - t) * s.color.a;
        const float radius = 1.4f + (1.0f - t) * 1.6f; // shrinks as it ages
        glm::mat4 model = glm::scale(
            glm::translate(glm::mat4(1.0f), s.pos),
            glm::vec3(radius, 1.0f, radius));
        glUniformMatrix4fv(circleModelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glm::vec4 col(s.color.r, s.color.g, s.color.b, alpha);
        glUniform4fv(circleColorLoc, 1, glm::value_ptr(col));
        glDrawArrays(GL_LINE_LOOP, 0, g_circleVertCount);
    }
    glBindVertexArray(0);
    glLineWidth(1.0f);
    glDisable(GL_BLEND);
}

// Visualize collision spheres (red) and bounding spheres (cyan) for every active
// scene object — driven by the two checkboxes in the Debug panel. Reuses the
// range-circle VAO and shader; renders three orthogonal great circles per object
// so the sphere reads correctly from any camera angle.
void renderDebugBounds() {
    if (!g_guiManager || !g_scene) return;
    const bool showColl   = g_guiManager->showCollisionBoxes;
    const bool showBounds = g_guiManager->showBoundingSpheres;
    if (!showColl && !showBounds) return;
    if (g_circleVAO == 0) return;

    rangeCircleShader.useShaderProgram();
    glm::mat4 view = myCamera.getViewMatrix();
    glUniformMatrix4fv(circleViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(circleProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.5f);
    glBindVertexArray(g_circleVAO);

    auto drawSphere = [&](const glm::vec3& center, float radius, const glm::vec4& color) {
        glUniform4fv(circleColorLoc, 1, glm::value_ptr(color));
        const glm::mat4 base = glm::translate(glm::mat4(1.0f), center);
        const glm::vec3 s(radius, 1.0f, radius);

        // XZ plane (default circle vertices live in XZ)
        glm::mat4 m = glm::scale(base, s);
        glUniformMatrix4fv(circleModelLoc, 1, GL_FALSE, glm::value_ptr(m));
        glDrawArrays(GL_LINE_LOOP, 0, g_circleVertCount);

        // XY plane: rotate the XZ ring 90° around the X axis
        m = glm::scale(glm::rotate(base, glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)), s);
        glUniformMatrix4fv(circleModelLoc, 1, GL_FALSE, glm::value_ptr(m));
        glDrawArrays(GL_LINE_LOOP, 0, g_circleVertCount);

        // YZ plane: rotate the XZ ring 90° around the Z axis
        m = glm::scale(glm::rotate(base, glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f)), s);
        glUniformMatrix4fv(circleModelLoc, 1, GL_FALSE, glm::value_ptr(m));
        glDrawArrays(GL_LINE_LOOP, 0, g_circleVertCount);
    };

    const glm::vec4 collColor   (1.0f, 0.25f, 0.25f, 0.9f); // red
    const glm::vec4 boundsColor (0.25f, 0.9f, 1.0f, 0.7f);  // cyan

    for (const auto& objPtr : g_scene->GetObjects()) {
        gps::SceneObject* obj = objPtr.get();
        if (!obj || !obj->IsActive()) continue;
        // Skip terrain tiles — their giant spheres drown out the rest.
        if (obj->GetTag() == "tile") continue;

        const glm::vec3 center = obj->GetWorldCenter();

        if (showColl && obj->GetCollisionRadius() > 0.0f)
            drawSphere(center, obj->GetCollisionRadius(), collColor);

        if (showBounds && obj->GetWorldRadius() > 0.0f)
            drawSphere(center, obj->GetWorldRadius(), boundsColor);
    }

    glBindVertexArray(0);
    glLineWidth(1.0f);
    glDisable(GL_BLEND);
}

// GAME LOOP
void processMovement() {
    auto& input = gps::InputManager::Instance();
    const bool wantsKbd   = g_guiManager && g_guiManager->WantsKeyboardInput();
    const bool wantsMouse = g_guiManager && g_guiManager->WantsMouseInput();

    // Edge-of-screen pan
    if (!wantsMouse) {
        glm::vec2 m = input.GetMousePosition();
        const float edge = 20.0f;
        const float es = cameraSpeed * deltaTime;
        if (m.x >= 0.0f && m.x < edge)
            myCamera.move(gps::MOVE_LEFT, es);
        else if (m.x > glWindowWidth - edge && m.x <= glWindowWidth)
            myCamera.move(gps::MOVE_RIGHT, es);
        if (m.y >= 0.0f && m.y < edge)
            myCamera.move(gps::MOVE_UP, es);
        else if (m.y > glWindowHeight - edge && m.y <= glWindowHeight)
            myCamera.move(gps::MOVE_DOWN, es);
    }

    // Middle-mouse drag pan — the world point under the cursor stays glued to it.
    if (middleDragging) {
        glm::vec3 cur = screenToWorld(input.GetMousePosition());
        if (dragHasPrev) {
            glm::vec3 worldDelta = cur - dragPrevWorld;
            glm::vec3 t = myCamera.getCameraTarget() - worldDelta;
            myCamera.centerOn(t);
        }
        // Resample after potentially moving the camera so we track incremental drag.
        dragPrevWorld = screenToWorld(input.GetMousePosition());
        dragHasPrev = true;
    }

    // Smooth zoom (ease toward target). Rebuild the ortho projection so the
    // change is visible — the per-frame uniform upload reads this matrix.
    zoomFactor += (targetZoomFactor - zoomFactor) * std::min(1.0f, deltaTime * 10.0f);
    {
        float aspectRatio = static_cast<float>(retina_width) / static_cast<float>(retina_height);
        float orthoSize   = 150.0f * zoomFactor;
        projection = glm::ortho(
            -orthoSize * aspectRatio, orthoSize * aspectRatio,
            -orthoSize, orthoSize,
            -1000.0f, 1000.0f
        );
    }

    // F-focus seek toward selection centroid.
    if (cameraSeeking) {
        glm::vec3 cur = myCamera.getCameraTarget();
        glm::vec3 goal(cameraSeekTargetXZ.x, cur.y, cameraSeekTargetXZ.z);
        glm::vec3 next = cur + (goal - cur) * std::min(1.0f, deltaTime * 6.0f);
        myCamera.centerOn(next);
        if (glm::length(goal - next) < 0.5f) cameraSeeking = false;
    }

    // Minimap click teleport (sets a fresh seek goal).
    if (g_guiManager) {
        glm::vec3 mp;
        if (g_guiManager->ConsumeMinimapClick(mp)) {
            cameraSeekTargetXZ = mp;
            cameraSeeking = true;
        }
    }

    // Clamp camera target to the tile grid so we can't pan into empty space.
    {
        glm::vec3 gMin, gMax;
        if (g_tileManager.GetGridBounds(gMin, gMax)) {
            glm::vec3 t = myCamera.getCameraTarget();
            glm::vec3 c = t;
            c.x = glm::clamp(t.x, gMin.x, gMax.x);
            c.z = glm::clamp(t.z, gMin.z, gMax.z);
            if (c.x != t.x || c.z != t.z) myCamera.centerOn(c);
        }
    }

    // Keyboard input (suppressed while ImGui has keyboard focus)
    if (wantsKbd) return;

    bool wasdHeld = false;
    if (input.IsKeyPressed(GLFW_KEY_W)) { myCamera.move(gps::MOVE_UP,    cameraSpeed * deltaTime); wasdHeld = true; }
    if (input.IsKeyPressed(GLFW_KEY_S)) { myCamera.move(gps::MOVE_DOWN,  cameraSpeed * deltaTime); wasdHeld = true; }
    if (input.IsKeyPressed(GLFW_KEY_A)) { myCamera.move(gps::MOVE_LEFT,  cameraSpeed * deltaTime); wasdHeld = true; }
    if (input.IsKeyPressed(GLFW_KEY_D)) { myCamera.move(gps::MOVE_RIGHT, cameraSpeed * deltaTime); wasdHeld = true; }
    if (wasdHeld) cameraSeeking = false; // user took manual control, cancel focus seek

    if (input.IsKeyJustPressed(GLFW_KEY_F1)) {
        if (g_guiManager) {
            g_guiManager->SetDebugPanelVisible(!g_guiManager->IsDebugPanelVisible());
        }
    }

    // F2 — alias for the help overlay (same as H).
    if (input.IsKeyJustPressed(GLFW_KEY_F2)) {
        if (g_guiManager) g_guiManager->ToggleHelpOverlay();
    }

    // H — toggle hotkey legend.
    if (input.IsKeyJustPressed(GLFW_KEY_H)) {
        if (g_guiManager) g_guiManager->ToggleHelpOverlay();
    }

    // P — pause / resume.
    if (input.IsKeyJustPressed(GLFW_KEY_P)) {
        if (g_guiManager) g_guiManager->TogglePaused();
    }

    // F — single press records time; second press within 0.3s frames the selection.
    if (input.IsKeyJustPressed(GLFW_KEY_F)) {
        double now = glfwGetTime();
        if (now - lastFPressTime < 0.3) {
            if (g_selectionSystem && g_scene) {
                const auto& ids = g_selectionSystem->GetSelectedIDs();
                if (!ids.empty()) {
                    glm::vec3 sum(0.0f);
                    int n = 0;
                    for (int id : ids) {
                        gps::SceneObject* o = g_scene->GetObjectByID(id);
                        if (o) { sum += o->GetTransform().GetPosition(); n++; }
                    }
                    if (n > 0) {
                        cameraSeekTargetXZ = sum / (float)n;
                        cameraSeeking = true;
                    }
                }
            }
        }
        lastFPressTime = now;
    }

    // Spawn trupe
    if (input.IsKeyJustPressed(GLFW_KEY_B)) {
        glm::vec3 spawnPos = g_sceneManager->GetTroopSpawnPosition();
        g_sceneManager->SpawnTroop(spawnPos);
    }

    // Spawn formație
    if (input.IsKeyJustPressed(GLFW_KEY_N)) {
        glm::vec3 spawnPos = glm::vec3(100.0f, -60.0f, -90.0f);
        g_sceneManager->SpawnTroopFormation(spawnPos, 10, 15.0f);
    }

    // Select all troops
    if (input.IsKeyJustPressed(GLFW_KEY_T)) {
        g_selectionSystem->SelectAllOfType(*g_scene, "Troop");
    }

    // Clear selection
    if (input.IsKeyJustPressed(GLFW_KEY_C)) {
        g_selectionSystem->ClearSelection();
    }

    // Quick cancel for placement mode without exiting app
    if (input.IsKeyJustPressed(GLFW_KEY_X)) {
        g_sceneManager->CancelPropPlacement();
        // Also cancel patrol-targeting and any active patrols on current selection.
        g_sceneManager->m_patrolTargeting  = false;
        g_sceneManager->m_patrolClickPhase = 0;
        g_sceneManager->m_patrolUnitIDs.clear();
        if (g_selectionSystem && g_scene) {
            for (int id : g_selectionSystem->GetSelectedIDs())
                if (gps::SceneObject* o = g_scene->GetObjectByID(id))
                    o->patrolData.isPatrolling = false;
        }
    }

    // Patrol hotkey: same entry point as the GUI button.
    if (input.IsKeyJustPressed(GLFW_KEY_Q)) {
        if (g_selectionSystem && g_selectionSystem->HasSelection()) {
            g_sceneManager->m_patrolUnitIDs.clear();
            for (int id : g_selectionSystem->GetSelectedIDs())
                g_sceneManager->m_patrolUnitIDs.push_back(id);
            g_sceneManager->m_patrolTargeting  = true;
            g_sceneManager->m_patrolClickPhase = 0;
        }
    }

    // Control groups: Ctrl+N saves current selection into slot N (0..9);
    // pressing N alone recalls slot N; Shift+N appends slot N to current.
    if (g_selectionSystem) {
        const bool ctrl  = input.IsKeyPressed(GLFW_KEY_LEFT_CONTROL) || input.IsKeyPressed(GLFW_KEY_RIGHT_CONTROL);
        const bool shift = input.IsKeyPressed(GLFW_KEY_LEFT_SHIFT)   || input.IsKeyPressed(GLFW_KEY_RIGHT_SHIFT);
        for (int k = 0; k <= 9; ++k) {
            if (!input.IsKeyJustPressed(GLFW_KEY_0 + k)) continue;
            if (ctrl) g_selectionSystem->SaveGroup(k);
            else      g_selectionSystem->RecallGroup(k, /*append=*/shift);
        }
    }

    if (input.IsKeyJustPressed(GLFW_KEY_F5)) {
        g_editorState->ToggleMode();
        g_selectionSystem->SetEditModeSelection(g_editorState->IsEditMode());
        g_selectionSystem->ClearSelection();

        if (g_editorState->IsEditMode()) {
            g_combatSystem->SetEnabled(false);
            // Stop all movement
            for (const auto& objPtr : g_scene->GetObjects()) {
                objPtr->movement.isMoving = false;
            }
            std::cout << "EDIT MODE enabled" << std::endl;
        } else {
            g_combatSystem->SetEnabled(true);
            std::cout << "PLAY MODE enabled" << std::endl;
        }
    }

    // Edit mode tool hotkeys
    if (g_editorState && g_editorState->IsEditMode()) {
        if (input.IsKeyJustPressed(GLFW_KEY_G)) {
            g_editorState->SetActiveTool(gps::EditTool::Translate);
        }
        if (input.IsKeyJustPressed(GLFW_KEY_R)) {
            g_editorState->SetActiveTool(gps::EditTool::Scale);
        }
    }
}

void updateDeltaTime() {
    double currentTime = glfwGetTime();
    deltaTime = static_cast<float>(currentTime - lastFrameTime);
    lastFrameTime = currentTime;
}

void renderScene(gps::Shader shader) {

    shader.useShaderProgram();

    if (!g_scene) return;


    glm::mat4 view = myCamera.getViewMatrix();


    // Render toate obiectele din scenă
    for (const auto& objPtr : g_scene->GetObjects()) {
        gps::SceneObject* obj = objPtr.get();

        if (!obj->GetModel() || !obj->IsActive()) continue;

        glm::mat4 modelMatrix = obj->GetTransform().GetModelMatrix();
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));

        glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(view * modelMatrix)));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

        // Highlight dacă e selectat
        glUniform1i(objectIDLoc, obj->GetID());

        if (isSelectedLoc != -1 && highlightColorLoc != -1) {
            int isSelected = g_selectionSystem->IsSelected(obj->GetID()) ? 1 : 0;
            glUniform1i(isSelectedLoc, isSelected);
            glm::vec3 hlColor = (g_editorState && g_editorState->IsEditMode())
                ? glm::vec3(0.0f, 0.8f, 1.0f)   // Cyan for edit mode
                : glm::vec3(1.0f, 1.0f, 0.0f);   // Yellow for play mode
            glUniform3fv(highlightColorLoc, 1, glm::value_ptr(hlColor));
        }

        // Per-object tint (e.g. red CIWS rounds)
        if (objectTintLoc != -1) {
            glm::vec3 tint = obj->projectileData.isProjectile
                ? obj->projectileData.tint
                : glm::vec3(1.0f);
            glUniform3fv(objectTintLoc, 1, glm::value_ptr(tint));
        }

        // Draw
        obj->GetModel()->Draw(shader);
    }

    // Ghost placement preview
    if (g_sceneManager->m_propPlacementMode && g_sceneManager) {
        gps::Model3D* ghostModel = g_sceneManager->GetModel(g_sceneManager->m_propPlacementModelName);
        if (ghostModel) {
            glm::vec2 mousePos = gps::InputManager::Instance().GetMousePosition();
            glm::vec3 ghostPos = screenToWorld(mousePos);

            glm::vec3 gridMin, gridMax;
            if (g_tileManager.GetPlayableBounds(gridMin, gridMax)) {
                ghostPos.x = glm::clamp(ghostPos.x, gridMin.x, gridMax.x);
                ghostPos.z = glm::clamp(ghostPos.z, gridMin.z, gridMax.z);
            }

            glm::mat4 ghostMatrix = glm::scale(
                glm::translate(glm::mat4(1.0f), ghostPos),
                g_sceneManager->m_propPlacementScale);
            glm::mat3 ghostNormal = glm::mat3(glm::transpose(glm::inverse(view * ghostMatrix)));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(ghostMatrix));
            glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(ghostNormal));
            glUniform1i(isSelectedLoc, 0);
            glUniform1i(isGhostLoc, 1);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);

            ghostModel->Draw(shader);

            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
            glUniform1i(isGhostLoc, 0);
        }
    }
}

void renderShadowMap() {
    depthMapShader.useShaderProgram();

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    // TODO: Render scene din perspectiva luminii

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, retina_width, retina_height);
}

void renderSkyBox() {
    skyboxShader.useShaderProgram();

    glm::mat4 view = myCamera.getViewMatrix();
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "view"),
        1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "projection"),
        1, GL_FALSE, glm::value_ptr(projection));

    mySkyBox.Draw(skyboxShader, view, projection);
}

glm::vec3 screenToWorld(const glm::vec2& screenPos) {
    // Convert screen coordinates to Normalized Device Coordinates (NDC)
    // NDC ranges from -1 to 1 in both x and y
    float x = (2.0f * screenPos.x) / glWindowWidth - 1.0f;
    float y = 1.0f - (2.0f * screenPos.y) / glWindowHeight;

    // Create a point in NDC space at the near plane
    glm::vec4 nearPointNDC = glm::vec4(x, y, -1.0f, 1.0f);

    // Transform from NDC to world space by inverting the view-projection matrix
    glm::mat4 view = myCamera.getViewMatrix();
    glm::mat4 inverseVP = glm::inverse(projection * view);

    // Unproject the near point to world space
    glm::vec4 nearPointWorld = inverseVP * nearPointNDC;
    if (nearPointWorld.w != 0.0f) {
        nearPointWorld /= nearPointWorld.w;
    }

    glm::vec3 rayDirection = myCamera.getCameraFrontDirection();

    // Find where this ray intersects the gameplay ground plane (y = kGroundY).
    // Ray equation: P = origin + t * direction
    // We want P.y = kGroundY, so: nearPointWorld.y + t * rayDirection.y = kGroundY
    float t = (kGroundY - nearPointWorld.y) / rayDirection.y;

    // Calculate the final intersection point
    glm::vec3 intersectionPoint = glm::vec3(nearPointWorld) + t * rayDirection;
    return intersectionPoint;
}

int main(int argc, const char* argv[]) {
    std::cout << "Starting GAME ENGINE" << std::endl;

    // Rulare headless a testelor functionale "LAB8_PG.exe --selftest".
    // Nu are nevoie de fereastra / OpenGL, deci ruleaza si iese inainte de init.
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--selftest") {
            gps::TestReport report = gps::SelfTest::RunFunctionalTests();
            gps::SelfTest::WriteReport(report);
            std::cout << report.ToString();
            return report.AllPassed() ? 0 : 1;
        }
    }

    // Init OpenGL
    if (!initOpenGLWindow()) {
        glfwTerminate();
        return 1;
    }

    setWindowCallbacks();
    initOpenGLState();

    // Init resources
    initShadowFrameBuffer();
    initModels();
    initShaders();
    initRangeCircle();
    initUniforms();
    initSkyBox();
   // initTileManager();

    initSceneManager();
    initSelectionSystem();
    initCollisionSystem();
    initCombatSystem();
    initGui();

    g_editorState = new gps::EditorState();
    g_guiManager->BindEditorState(g_editorState);

    // Benchmark harness (capitolul 6): spawneaza incarcarea de test si masoara
    // performanta. Condus din sectiunea "Benchmark & Testare" a panoului Debug.
    g_benchmark = new gps::BenchmarkHarness();
    g_benchmark->Initialize(g_scene, g_sceneManager, &g_tileManager, g_selectionSystem);
    g_guiManager->BindBenchmark(g_benchmark);

    // Bases + MVG enemy seed live across the match lifecycle; spawn once at
    // boot and on Reset.
    spawnBases();
    SpawnEnemyStartingForce();

    // Game loop
    lastFrameTime = glfwGetTime();

    while (!glfwWindowShouldClose(glWindow)) {
        // Update
        gps::InputManager::Instance().Update();
        updateDeltaTime();
        glfwPollEvents();
        processMovement();

        if (g_benchmark) g_benchmark->Update(deltaTime);

        // Update scene
        if (g_scene) {
            g_scene->Update(deltaTime);
        }

        const bool paused = g_guiManager && g_guiManager->IsPaused();
        if (!paused && (!g_editorState || g_editorState->IsPlayMode())) {

        if (g_scene) {
            for (auto* obj : g_scene->GetObjectsRaw()) {
                if (!obj->IsActive()) continue;
                const gps::UnitStats& s = obj->unitStats;
                if (s.productionRate > 0.0f && !s.resourceType.empty() && s.resourceType != "none") {
                    float bonus = 1.0f;
                    bool onMatchingTile = false;
                    int gx, gz;
                    g_tileManager.WorldToGrid(obj->GetTransform().GetPosition(), gx, gz);
                    if (gps::Tile* t = g_tileManager.GetTile(gx, gz)) {
                        const gps::TileEffect& e = gps::GetTileEffect(t->type);
                        if (e.resource == s.resourceType) {
                            bonus = e.productionBonus;
                            onMatchingTile = true;
                        }
                    }
                    // Fish boats only generate fish when parked on a Fish tile.
                    if (obj->GetTag() == "fishBoat" && !onMatchingTile) continue;
                    gps::ResourceManager::Instance().Deposit(s.resourceType, s.productionRate * bonus * deltaTime);
                }
            }
        }

        // Match flow: kick off the match the first frame the user clicks Start.
        if (g_guiManager && g_guiManager->IsGameStarted() && !g_matchInitialized) {
            g_matchInitialized = true;
            g_matchActive      = true;
            g_matchStartTime   = glfwGetTime();
        }

        const bool inGrace = g_matchActive
                            && (glfwGetTime() - g_matchStartTime) < kMatchGraceSec;
        if (g_combatSystem && g_matchActive && !inGrace) {
            g_combatSystem->Update(deltaTime);
            for (int deadID : g_combatSystem->GetDeadIDs()) {
                if (gps::SceneObject* o = g_scene ? g_scene->GetObjectByID(deadID) : nullptr) {
                    if      (o->unitStats.faction == 1) g_matchP1Lost++;
                    else if (o->unitStats.faction == 2) g_matchP2Lost++;
                }
                if (deadID == g_baseP1ID) g_baseP1ID = -2;
                if (deadID == g_baseP2ID) g_baseP2ID = -2;
                if (g_selectionSystem) g_selectionSystem->RemoveFromSelection(deadID);
                if (g_scene) g_scene->DestroyObject(deadID);
            }
            g_combatSystem->ClearDeadIDs();

            // Win condition: whichever base survives wins.
            if (g_baseP1ID == -2 || g_baseP2ID == -2) {
                g_matchActive = false;
                if (g_guiManager) {
                    g_guiManager->SetVictory(g_baseP2ID == -2);
                    g_guiManager->SetDefeat (g_baseP1ID == -2);
                }
            }
        }

        // Enemy AI — every kEnemyOrderInterval seconds (after grace ends),
        // dispatch up to 2 idle enemy combat ships toward the P1 base. Reuses
        // MovementData verbatim; existing CombatSystem handles engagement when
        // they come into range.
        {
            static double s_lastEnemyOrder = 0.0;
            constexpr double kEnemyOrderInterval = 25.0;
            const double now = glfwGetTime();
            const bool   postGrace = (now - g_matchStartTime) >= kMatchGraceSec;
            if (g_matchActive && postGrace && g_baseP1ID >= 0
                && (now - s_lastEnemyOrder) > kEnemyOrderInterval)
            {
                s_lastEnemyOrder = now;
                gps::SceneObject* p1Base = g_scene->GetObjectByID(g_baseP1ID);
                if (p1Base) {
                    const glm::vec3 target = p1Base->GetWorldCenter();
                    int dispatched = 0;
                    for (const auto& objPtr : g_scene->GetObjects()) {
                        if (dispatched >= 2) break;
                        gps::SceneObject* o = objPtr.get();
                        if (!o->IsActive() || !o->unitStats.isAlive) continue;
                        if (o->unitStats.faction != 2)             continue;
                        if (!o->unitStats.isMovable)               continue;
                        if (!o->unitStats.isCombatUnit)            continue;
                        if (o->movement.isMoving)                  continue;
                        if (o->patrolData.isPatrolling)            continue;
                        if (o->orbitData.isOrbiting)               continue;

                        glm::vec3 src = o->GetTransform().GetPosition();
                        glm::vec3 dir = target - src;
                        dir.y = 0.0f;
                        const float dist = glm::length(dir);
                        if (dist < o->unitStats.attackRange) continue; // already in range
                        if (dist < 0.0001f) continue;
                        dir /= dist;
                        // Stop just outside attack range so the unit can fire.
                        glm::vec3 dest = target - dir * (o->unitStats.attackRange * 0.8f);

                        o->movement.isMoving      = true;
                        o->movement.moveStartPos  = src;
                        o->movement.moveEndPos    = dest;
                        o->movement.moveStartTime = static_cast<float>(now);
                        o->movement.moveDuration  = 4.0f;
                        o->movement.moveDirection = dir;
                        o->unitStats.targetID     = g_baseP1ID;
                        ++dispatched;
                    }
                }
            }
        }

        // Carrier deploy/respawn: when a carrier's plane is gone, schedule a
        // replacement; when the timer expires (and the carrier is still alive),
        // spawn the new plane via the same helper used at initial placement.
        {
            const double now = glfwGetTime();
            for (const auto& objPtr : g_scene->GetObjects()) {
                gps::SceneObject* carrier = objPtr.get();
                if (!carrier->IsActive() || !carrier->unitStats.isAlive) continue;
                if (carrier->GetTag() != "aircraftCarrier") continue;

                // Has the tracked plane gone missing / died?
                if (carrier->orbitData.childAircraftID >= 0) {
                    gps::SceneObject* plane = g_scene->GetObjectByID(carrier->orbitData.childAircraftID);
                    if (!plane || !plane->IsActive() || !plane->unitStats.isAlive) {
                        carrier->orbitData.childAircraftID = -1;
                        carrier->orbitData.respawnAfter    = now + kAircraftRespawnDelay;
                    }
                }
                // Timer expired -> deploy a fresh plane.
                if (carrier->orbitData.childAircraftID < 0 &&
                    carrier->orbitData.respawnAfter > 0.0 &&
                    now >= carrier->orbitData.respawnAfter)
                {
                    SpawnCarrierAircraft(carrier);
                }
            }
        }

        // Orbit update: aircraft circles around parent carrier using circle equation
        {
            std::vector<int> deadAircraftIDs;
            for (const auto& objPtr : g_scene->GetObjects()) {
                gps::SceneObject* obj = objPtr.get();
                if (!obj->IsActive() || !obj->orbitData.isOrbiting) continue;

                gps::SceneObject* parent = g_scene->GetObjectByID(obj->orbitData.parentID);
                if (!parent || !parent->IsActive() || !parent->unitStats.isAlive) {
                    obj->unitStats.isAlive = false;
                    obj->SetActive(false);
                    deadAircraftIDs.push_back(obj->GetID());
                    continue;
                }

                obj->orbitData.orbitAngle += obj->orbitData.orbitSpeed * deltaTime;

                float a = obj->orbitData.orbitAngle;
                float r = obj->orbitData.orbitRadius;
                glm::vec3 center = parent->GetTransform().GetPosition();

                obj->GetTransform().SetPosition(glm::vec3(
                    center.x + r * std::cos(a),
                    center.y + obj->orbitData.orbitHeight,
                    center.z + r * std::sin(a)
                ));
                obj->GetTransform().SetRotation(glm::vec3(0.0f, -glm::degrees(a) + 180, 0.0f));
                obj->UpdateWorldBounds();
            }
            for (int id : deadAircraftIDs)
                g_scene->DestroyObject(id);
        }

        // Update troops movement
        double currentTime = glfwGetTime();
        glm::vec3 gridBoundsMin, gridBoundsMax;
        bool hasGridBounds = g_tileManager.GetPlayableBounds(gridBoundsMin, gridBoundsMax);

        // Movement loop
        for (const auto& objPtr : g_scene->GetObjects()) {
            gps::SceneObject* obj = objPtr.get();

            if (!obj->movement.isMoving) continue;

            glm::vec3 oldPos = obj->GetTransform().GetPosition();

            float elapsed = static_cast<float>(currentTime) - obj->movement.moveStartTime;
            float alpha = elapsed / obj->movement.moveDuration;

            if (alpha >= 1.0f) {
                alpha = 1.0f;
                obj->movement.isMoving = false;
            }

            glm::vec3 newPos = glm::mix(obj->movement.moveStartPos, obj->movement.moveEndPos, alpha);

            // Clamp to tile grid bounds (skip for projectiles)
            if (hasGridBounds && !obj->projectileData.isProjectile) {
                newPos.x = glm::clamp(newPos.x, gridBoundsMin.x, gridBoundsMax.x);
                newPos.z = glm::clamp(newPos.z, gridBoundsMin.z, gridBoundsMax.z);
            }

            obj->GetTransform().SetPosition(newPos);

            if (glm::length(obj->movement.moveDirection) > 0.0001f) {
                float yaw = glm::degrees(std::atan2(obj->movement.moveDirection.x, -obj->movement.moveDirection.z));
                glm::vec3 currentRot = obj->GetTransform().GetRotation();
                obj->GetTransform().SetRotation(glm::vec3(currentRot.x, -yaw, currentRot.z));
            }

            obj->UpdateWorldBounds();

            // Land tiles, the decorative border ring, and the void outside any
            // tile all block ships: revert and halt at the boundary.
            if (!obj->projectileData.isProjectile) {
                int gx, gz;
                g_tileManager.WorldToGrid(newPos, gx, gz);
                gps::Tile* t = g_tileManager.GetTile(gx, gz);
                if (!t || t->isBorder || t->type == gps::TileType::Land) {
                    obj->GetTransform().SetPosition(oldPos);
                    obj->movement.isMoving = false;
                    obj->UpdateWorldBounds();
                    continue;
                }
            }

            // Sphere-sphere collisions: slide along the contact instead of locking up.
            if (g_collisionSystem && !obj->projectileData.isProjectile) {
                gps::SceneObject* other = g_collisionSystem->GetCollidingObject(obj);
                // Same-faction units phase through each other so troops don't
                // deadlock brushing past one another. Friendly mines are also
                // phased through (they only arm on enemy contact), so a boat can
                // sail over its own minefield — but other immovable buildings
                // (turrets, oil rigs, bases) stay solid.
                if (other && (other->unitStats.isMovable || other->GetTag() == "mine")
                          && obj->unitStats.faction != 0
                          && obj->unitStats.faction == other->unitStats.faction) {
                    other = nullptr;
                }
                if (other) {
                    glm::vec3 n = obj->GetWorldCenter() - other->GetWorldCenter();
                    n.y = 0.0f;
                    float nl = glm::length(n);
                    if (nl > 0.0001f) {
                        n /= nl;
                        glm::vec3 move = newPos - oldPos;
                        glm::vec3 slide = move - glm::dot(move, n) * n;
                        glm::vec3 slidePos = oldPos + slide;
                        obj->GetTransform().SetPosition(slidePos);
                        obj->UpdateWorldBounds();
                        if (g_collisionSystem->GetCollidingObject(obj)) {
                            // Slide didn't free us — fall back to halting.
                            obj->GetTransform().SetPosition(oldPos);
                            obj->UpdateWorldBounds();
                            obj->movement.isMoving = false;
                        } else {
                            // Re-anchor the move so we continue toward the goal from here.
                            obj->movement.moveStartPos  = slidePos;
                            obj->movement.moveStartTime = static_cast<float>(currentTime);
                        }
                    } else {
                        // Centers coincide — degenerate, halt.
                        obj->GetTransform().SetPosition(oldPos);
                        obj->UpdateWorldBounds();
                        obj->movement.isMoving = false;
                    }
                }
            }
        }

        // Patrol re-arm: any unit that just halted while still patrolling flips
        // its leg direction and arms a fresh move toward the other endpoint.
        // Runs after the movement loop so collision halts also trigger a retry.
        for (const auto& objPtr : g_scene->GetObjects()) {
            gps::SceneObject* obj = objPtr.get();
            if (!obj->IsActive() || !obj->unitStats.isAlive) continue;
            if (!obj->patrolData.isPatrolling || obj->movement.isMoving) continue;

            obj->patrolData.headingToB = !obj->patrolData.headingToB;
            glm::vec3 target = (obj->patrolData.headingToB
                                  ? obj->patrolData.pointB
                                  : obj->patrolData.pointA)
                               + obj->patrolData.offsetFromCentroid;
            if (hasGridBounds) {
                target.x = glm::clamp(target.x, gridBoundsMin.x, gridBoundsMax.x);
                target.z = glm::clamp(target.z, gridBoundsMin.z, gridBoundsMax.z);
            }
            glm::vec3 startPos = obj->GetTransform().GetPosition();
            glm::vec3 dir = target - startPos;
            dir.y = 0.0f;
            if (glm::length(dir) > 0.0001f) obj->movement.moveDirection = glm::normalize(dir);

            obj->movement.isMoving      = true;
            obj->movement.moveStartPos  = startPos;
            obj->movement.moveEndPos    = target;
            obj->movement.moveStartTime = static_cast<float>(currentTime);
            obj->movement.moveDuration  = 2.0f;
            int dgx, dgz;
            g_tileManager.WorldToGrid(target, dgx, dgz);
            if (gps::Tile* dt = g_tileManager.GetTile(dgx, dgz))
                obj->movement.moveDuration *= gps::GetTileEffect(dt->type).moveDurationMul;
        }

        // Naval mine detonation: any movable non-friendly unit within trigger
        // range arms the mine, which then applies splash damage in a wider
        // radius and self-destructs. Other mines are skipped from both the
        // trigger and the splash so a single hit can't chain through a field.
        {
            constexpr float kMineDetonateRadius = 25.0f;
            constexpr float kMineSplashRadius   = 35.0f;
            constexpr int   kMineDamage         = 80;

            std::vector<int> minesToDestroy;
            std::vector<int> mineKilled;
            for (const auto& objPtr : g_scene->GetObjects()) {
                gps::SceneObject* mine = objPtr.get();
                if (!mine->IsActive() || !mine->unitStats.isAlive) continue;
                if (mine->GetTag() != "mine") continue;

                const glm::vec3 minePos    = mine->GetWorldCenter();
                const int       mineFaction = mine->unitStats.faction;

                bool triggered = false;
                for (const auto& vPtr : g_scene->GetObjects()) {
                    gps::SceneObject* candidate = vPtr.get();
                    if (!candidate->IsActive() || !candidate->unitStats.isAlive) continue;
                    if (!candidate->unitStats.isMovable) continue;
                    if (candidate->projectileData.isProjectile) continue;
                    if (candidate->GetTag() == "mine") continue;
                    if (candidate->GetID() == mine->GetID()) continue;
                    if (mineFaction != 0 && candidate->unitStats.faction == mineFaction) continue;
                    if (glm::distance(candidate->GetWorldCenter(), minePos) <= kMineDetonateRadius) {
                        triggered = true;
                        break;
                    }
                }
                if (!triggered) continue;

                // Splash: same skip rules as the trigger scan; reuses the shared
                // damage helper so kills feed the standard cleanup pipeline.
                for (const auto& vPtr : g_scene->GetObjects()) {
                    gps::SceneObject* victim = vPtr.get();
                    if (!victim->IsActive()) continue;
                    if (victim->projectileData.isProjectile) continue;
                    if (victim->GetTag() == "mine") continue;
                    if (victim->GetID() == mine->GetID()) continue;
                    if (mineFaction != 0 && victim->unitStats.faction == mineFaction) continue;
                    if (glm::distance(victim->GetWorldCenter(), minePos) > kMineSplashRadius) continue;
                    ApplyDamageTo(victim, kMineDamage, mine->GetID(), mineKilled);
                }
                // Bigger, redder burst for a mine — it's a hard explosion.
                EmitSparks(minePos, 24, glm::vec4(1.0f, 0.30f, 0.10f, 1.0f), 50.0f, 0.7f);
                minesToDestroy.push_back(mine->GetID());
            }
            CleanupKilled(mineKilled);
            for (int id : minesToDestroy) {
                if (g_selectionSystem) g_selectionSystem->RemoveFromSelection(id);
                g_scene->DestroyObject(id);
            }

            // Mine kills can also drop a base (rare, but possible).
            if (g_matchActive && (g_baseP1ID == -2 || g_baseP2ID == -2)) {
                g_matchActive = false;
                if (g_guiManager) {
                    g_guiManager->SetVictory(g_baseP2ID == -2);
                    g_guiManager->SetDefeat (g_baseP1ID == -2);
                }
            }
        }

        // Projectile arrival: apply damage and destroy stopped projectiles
        {
            std::vector<int> toDestroy;
            std::vector<int> killed;
            for (const auto& objPtr : g_scene->GetObjects()) {
                gps::SceneObject* obj = objPtr.get();
                if (!obj->IsActive() || !obj->projectileData.isProjectile || obj->movement.isMoving) continue;

                const int   dmg    = (int)obj->projectileData.damage;
                const float splash = obj->projectileData.splashRadius;
                if (splash > 0.0f) {
                    // AOE: damage everything in radius around the impact point,
                    // skipping owner, projectiles, and same-faction units.
                    const glm::vec3 impact = obj->GetWorldCenter();
                    gps::SceneObject* owner = g_scene->GetObjectByID(obj->projectileData.ownerID);
                    const int ownerFaction = owner ? owner->unitStats.faction : 0;
                    for (const auto& vPtr : g_scene->GetObjects()) {
                        gps::SceneObject* victim = vPtr.get();
                        if (!victim->IsActive()) continue;
                        if (victim->projectileData.isProjectile) continue;
                        if (victim->GetID() == obj->projectileData.ownerID) continue;
                        const int vf = victim->unitStats.faction;
                        if (ownerFaction != 0 && vf != 0 && vf == ownerFaction) continue;
                        if (glm::distance(victim->GetWorldCenter(), impact) > splash) continue;
                        ApplyDamageTo(victim, dmg, obj->projectileData.ownerID, killed);
                    }
                    // Big AOE burst for splash impact.
                    EmitSparks(impact, 16, glm::vec4(1.0f, 0.55f, 0.15f, 1.0f), 38.0f, 0.55f);
                } else {
                    gps::SceneObject* tgt = g_scene->GetObjectByID(obj->projectileData.targetID);
                    if (tgt) EmitSparks(tgt->GetWorldCenter(), 10,
                                        glm::vec4(1.0f, 0.65f, 0.20f, 1.0f), 28.0f, 0.45f);
                    ApplyDamageTo(tgt, dmg, obj->projectileData.ownerID, killed);
                }
                toDestroy.push_back(obj->GetID());
            }
            CleanupKilled(killed);
            for (int id : toDestroy) g_scene->DestroyObject(id);

            // Win condition: same check as after the CombatSystem update, run
            // again here because projectile damage is resolved further down.
            if (g_matchActive && (g_baseP1ID == -2 || g_baseP2ID == -2)) {
                g_matchActive = false;
                if (g_guiManager) {
                    g_guiManager->SetVictory(g_baseP2ID == -2);
                    g_guiManager->SetDefeat (g_baseP1ID == -2);
                }
            }
        }

        } // end play-mode-only systems

    myCustomShader.useShaderProgram();
    
    glm::mat4 view = myCamera.getViewMatrix();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glm::vec3 pointLightPosEye = glm::vec3(view * glm::vec4(lightDir, 1.0f));
    glUniform3fv(lightPosEyeLoc, 1, glm::value_ptr(pointLightPosEye));

    glm::vec3 dirLightEye = glm::inverseTranspose(glm::mat3(view)) * dirLightWorld;
    glUniform3fv(dirLightDirEyeLoc, 1, glm::value_ptr(dirLightEye));

    glm::mat4 invView = glm::inverse(view);
    glm::vec3 cameraPosWorld = glm::vec3(invView[3]);
    glm::vec3 cameraPosEye = glm::vec3(view * glm::vec4(cameraPosWorld, 1.0f));
    glUniform3fv(viewPosEyeLoc, 1, glm::value_ptr(cameraPosEye));


    if (g_guiManager) {
        g_guiManager->SetDeltaTime(deltaTime);
        g_guiManager->SetCameraPosition(myCamera.getCameraPosition());
        g_guiManager->SetZoomFactor(zoomFactor);
        g_guiManager->SetViewProjection(view, projection);

        // Roll up match stats for the end-game overlay.
        int p1Alive = 0, p2Alive = 0;
        if (g_scene) {
            for (const auto& o : g_scene->GetObjects()) {
                gps::SceneObject* obj = o.get();
                if (!obj || !obj->IsActive() || !obj->unitStats.isAlive) continue;
                const std::string& tag = obj->GetTag();
                if (tag == "tile" || tag == "tileBorder") continue;
                if      (obj->unitStats.faction == 1) p1Alive++;
                else if (obj->unitStats.faction == 2) p2Alive++;
            }
        }
        const float elapsed = g_matchInitialized
            ? static_cast<float>(glfwGetTime() - g_matchStartTime)
            : 0.0f;
        g_guiManager->SetMatchStats(elapsed, p1Alive, p2Alive,
                                    g_matchP1Lost, g_matchP2Lost, g_matchOilSpent);

        // Surface the grace-period countdown for the centered overlay.
        const float graceLeft = g_matchActive
            ? static_cast<float>(kMatchGraceSec - (glfwGetTime() - g_matchStartTime))
            : 0.0f;
        g_guiManager->SetGraceSecRemaining(graceLeft > 0.0f ? graceLeft : 0.0f);

        if (g_guiManager->ConsumeResetRequest()) resetMatch();
    }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // Update uniforms
        myCustomShader.useShaderProgram();

        //view = myCamera.getViewMatrix();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));


        glm::vec3 cameraPos = myCamera.getCameraPosition();
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));

        // Render
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render shadow map
        //renderShadowMap();

        // Render scene
        renderScene(myCustomShader);

        // Render range circles for selected combat units
        renderRangeCircles();

        // Damage feedback sparks (re-uses the range-circle shader).
        renderSparks();

        // Debug overlay: collision spheres / model bounds (toggled in Debug panel)
        renderDebugBounds();

        // Render skybox
        renderSkyBox();

        myCustomShader.useShaderProgram();



        // Render selection box (UI overlay)
        if (g_selectionSystem) {
            g_selectionSystem->Render();
        }

        if (g_guiManager) {
            g_guiManager->BeginFrame();
            g_guiManager->RenderAllPanels();
            g_guiManager->EndFrame();
        }


        // Swap buffers
        glfwSwapBuffers(glWindow);
    }

    // Cleanup
    delete g_benchmark;
    delete g_editorState;
    delete g_guiManager;
    delete g_scene;
    delete g_sceneManager;
    delete g_selectionSystem;
    delete g_collisionSystem;
    delete g_combatSystem;

    glfwDestroyWindow(glWindow);
    glfwTerminate();
    return 0;
}