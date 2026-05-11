//
// main.cpp - INTEGRAT CU NOILE SISTEME
// Exemplu complet de utilizare: InputManager, Scene, SceneManager, SelectionSystem
//

#include <iostream>
#include <cmath>
#include <string>

// OpenGL/GLFW/GLEW
#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp> 

// Clase existente
#include "Shader.hpp"
#include "Camera.hpp"
#include "CameraIso.hpp"
#include "Model3D.hpp"
#include "SkyBox.hpp"
#include "RayCaster.hpp"
#include "TileManager.hpp"

// NOILE SISTEME
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
// ===========================
// WINDOW SETTINGS
// ===========================
int glWindowWidth = 1920;
int glWindowHeight = 1080;
int retina_width, retina_height;
GLFWwindow* glWindow = NULL;

// ===========================
// CAMERA & VIEW
// ===========================
gps::Camera myCamera(
    glm::vec3(0.0f, 200, -200)
);

float cameraSpeed = 150.0f;
float zoomFactor = 1.0f;

// ===========================
// SHADERS
// ===========================
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

// ===========================
// LIGHTING
// ===========================
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

// ===========================
// MODELE 3D (încărcate manual)
// ===========================
gps::Model3D sceneModel;      // Teren
gps::Model3D obiecte;          // Obiecte statice
gps::Model3D orcModel;         // Trupe
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

// Hex tile models (objects/tiles/*.obj). Center-to-vertex = 5.587 in model units.
gps::Model3D waterTile;
gps::Model3D oilTile;
gps::Model3D fishTile;
gps::Model3D desertTile;
gps::Model3D greenTile;
gps::Model3D fishBoat;

// ===========================
// SKYBOX
// ===========================
gps::SkyBox mySkyBox;
std::vector<const GLchar*> faces{
    "textures/skybox/rt.tga",
    "textures/skybox/lf.tga",
    "textures/skybox/up.tga",
    "textures/skybox/dn.tga",
    "textures/skybox/bk.tga",
    "textures/skybox/ft.tga"
};

// ===========================
// TILE MANAGER
// ===========================
// Hex tile model has center-to-vertex = 5.587 in model units; scaled by 27 -> ~150.85 world units.
// m_tileSize must equal world center-to-vertex for hexes to pack flush.
gps::TileManager g_tileManager(150.85f, glm::vec3(27.0f), -60);

// ===========================
// NOILE SISTEME (GLOBALE)
// ===========================
gps::Scene* g_scene = nullptr;
gps::SceneManager* g_sceneManager = nullptr;
gps::SelectionSystem* g_selectionSystem = nullptr;
gps::CollisionSystem* g_collisionSystem = nullptr;
gps::CombatSystem*   g_combatSystem    = nullptr;
gps::ResourceManager& resourceManager = gps::ResourceManager::Instance();
gps::GuiManager* g_guiManager = nullptr;
gps::EditorState* g_editorState = nullptr;

// ===========================
// PROP PLACEMENT MODE (MVP)
// ===========================
int g_nextPlacedPropID = 10001;


// ===========================
// TIME
// ===========================
double lastFrameTime = 0.0;
float deltaTime = 1.0f;

// ===========================
// PROJECTION
// ===========================
glm::mat4 projection;

// ===========================
// FUNCTION DECLARATIONS
// ===========================
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
void renderDebugBounds();

glm::vec3 screenToWorld(const glm::vec2& screenPos);

// ===========================
// CALLBACKS IMPLEMENTATION
// ===========================

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

    if (g_guiManager && g_guiManager->WantsKeyboardInput()) {
        return;
    }

    // ESC pentru exit
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    // Forward către InputManager
    gps::InputManager::Instance().OnMouseButton(button, action, mods);

    if (g_guiManager && g_guiManager->WantsMouseInput()) {
        return;
    }

    if (!g_selectionSystem || !g_scene) return;

    glm::vec2 mousePos = gps::InputManager::Instance().GetMousePosition();

    // ========== EDIT MODE INPUT ==========
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

    // ========== PLAY MODE INPUT (existing code) ==========

    // Placement mode: left click places selected prop and skips selection box logic.
    if (g_sceneManager->m_propPlacementMode && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        if (g_sceneManager && resourceManager.Get("Oil") >= 50) {
            glm::vec3 worldPos = screenToWorld(mousePos);

            resourceManager.Spend("Oil", 50);

            glm::vec3 gridMin, gridMax;
            if (g_tileManager.GetGridBounds(gridMin, gridMax)) {
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

            if (spawned) {
                std::cout << "Placed " << g_sceneManager->m_propPlacementLabel << " at ("
                    << worldPos.x << ", " << worldPos.y << ", " << worldPos.z << ")" << std::endl;

                if (spawned->GetTag() == "aircraftCarrier") {
                    glm::vec3 aircraftPos = worldPos + glm::vec3(spawned->unitStats.attackRange, 20.0f, 0.0f);
                    gps::SceneObject* plane = g_sceneManager->SpawnObject(
                        "Aircraft_for_" + std::to_string(spawned->GetID()),
                        "aircraft",
                        "aircraft",
                        aircraftPos,
                        glm::vec3(3.0f)
                    );
                    if (plane) {
                        plane->orbitData.isOrbiting = true;
                        plane->orbitData.parentID = spawned->GetID();
                        plane->orbitData.orbitRadius = spawned->unitStats.attackRange;
                        plane->orbitData.orbitSpeed = 1.0f;
                        plane->orbitData.orbitAngle = 0.0f;
                        plane->orbitData.orbitHeight = 20.0f;
                        plane->SetCollisionRadius(0.0f);
                    }
                }
            }
        }
        return;
    }

    // LEFT MOUSE - Selection
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

    // RIGHT MOUSE - Cancel placement mode
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS && g_sceneManager->m_propPlacementMode) {
        g_sceneManager->CancelPropPlacement();
        return;
    }

    // RIGHT MOUSE - Movement Command
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        glm::vec3 worldPos = screenToWorld(mousePos);

        const auto& selectedIDs = g_selectionSystem->GetSelectedIDs();
        if (selectedIDs.empty()) return;

        glm::vec3 gridMin, gridMax;
        bool hasGrid = g_tileManager.GetGridBounds(gridMin, gridMax);

        int numSelected = selectedIDs.size();
        int columns = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(numSelected))));
        float spacing = 15.0f;

        int idx = 0;
        for (int troopID : selectedIDs) {
            gps::SceneObject* obj = g_scene->GetObjectByID(troopID);
            if (!obj) continue;
            if (!obj->unitStats.isMovable) continue;
            int row = idx / columns;
            int col = idx % columns;
            float offsetX = (col - columns / 2.0f) * spacing;
            float offsetZ = (row - numSelected / columns / 2.0f) * spacing;

            glm::vec3 formationPos = worldPos + glm::vec3(offsetX, 0.0f, offsetZ);

            if (hasGrid) {
                formationPos.x = glm::clamp(formationPos.x, gridMin.x, gridMax.x);
                formationPos.z = glm::clamp(formationPos.z, gridMin.z, gridMax.z);
            }

            glm::vec3 startPos = obj->GetTransform().GetPosition();
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

            idx++;
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

    // Zoom
    zoomFactor *= (1.0f - static_cast<float>(yoffset) * 0.1f);
    zoomFactor = glm::clamp(zoomFactor, 0.5f, 2.0f);

    // Update projection
    windowResizeCallback(window, glWindowWidth, glWindowHeight);
}

//
// GUI INIT
//

void initGui() {
    g_guiManager = new gps::GuiManager();
    g_guiManager->Initialize(glWindow, "#version 410");

    // Conecteaza sistemele existente
    g_guiManager->BindSystems(g_scene, g_sceneManager, g_selectionSystem, &g_tileManager);

    // ─── Adauga butoane custom (optional) ───


 
    /*
    // Buton: Toggle Spawn
    gps::GuiButton toggleSpawnBtn;
    toggleSpawnBtn.label = "🔄 Toggle Spawn";
    toggleSpawnBtn.tooltip = "Activeaza/dezactiveaza spawn-ul";
    toggleSpawnBtn.callback = [&]() {
        bool current = g_sceneManager->IsSpawnEnabled();
        g_sceneManager->SetSpawnEnabled(!current);
        std::cout << "Spawn " << (!current ? "ENABLED" : "DISABLED") << std::endl;
        };
    g_guiManager->AddButton(toggleSpawnBtn);
    */
    std::cout << "✅ GUI initialized" << std::endl;
}

// ===========================
// INITIALIZATION
// ===========================

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
    std::cout << "📦 Loading models..." << std::endl;

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
    waterTile.LoadModel("objects/tiles/waterTile.obj", "textures/");
    oilTile.LoadModel("objects/tiles/oilTile.obj", "textures/");
    fishTile.LoadModel("objects/tiles/fishTile.obj", "textures/");
    desertTile.LoadModel("objects/tiles/desertTile.obj", "textures/");
    greenTile.LoadModel("objects/tiles/greenTile.obj", "textures/");
    fishBoat.LoadModel("objects/tiles/fishBoat.obj", "textures/");

    std::cout << "✅ Models loaded" << std::endl;
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

/*
void initTileManager() {
    g_tileManager.init(100, 10.0f, glm::vec3(0.0f, -60.0f, 0.0f));
    std::cout << "✅ TileManager initialized" << std::endl;
}
*/
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
    g_sceneManager->RegisterModel("fishBoat", &fishBoat);

    g_tileManager.Initialize(&waterTile, g_scene);
    g_tileManager.RegisterTileModel(gps::TileType::Sea,      &waterTile);
    g_tileManager.RegisterTileModel(gps::TileType::Oil,      &oilTile);
    g_tileManager.RegisterTileModel(gps::TileType::Fish,     &fishTile);
    g_tileManager.RegisterTileModel(gps::TileType::Shallows, &desertTile);
    g_tileManager.RegisterTileModel(gps::TileType::Land,     &greenTile);
    // Generate initial 3x3 tile grid centered at origin
    g_tileManager.GenerateAndLoadGrid(-2, 1, -1, 2);

    // 4. Setup scena
    g_sceneManager->SetupScene();

    // 5. Enable spawn
    g_sceneManager->SetSpawnEnabled(true);

    std::cout << "✅ SceneManager initialized with "
        << g_scene->GetObjectCount() << " objects" << std::endl;
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



// ===========================
// RANGE CIRCLE
// ===========================

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

    std::cout << "✅ Range circle initialized" << std::endl;
}

void renderRangeCircles() {
    if (!g_scene || !g_selectionSystem || !g_selectionSystem->HasSelection()) return;
    if (g_circleVAO == 0) return;

    rangeCircleShader.useShaderProgram();

    glm::mat4 view = myCamera.getViewMatrix();
    glUniformMatrix4fv(circleViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(circleProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2.0f);

    glBindVertexArray(g_circleVAO);

    for (int id : g_selectionSystem->GetSelectedIDs()) {
        gps::SceneObject* obj = g_scene->GetObjectByID(id);
        if (!obj || !obj->IsActive()) continue;
        if (!obj->unitStats.isCombatUnit || obj->unitStats.attackRange <= 0.0f) continue;

        glm::vec3 pos = obj->GetTransform().GetPosition();
        pos.y += 2.0f; // lift slightly above water to avoid z-fighting
        float range = obj->unitStats.attackRange;

        glm::mat4 model = glm::scale(
            glm::translate(glm::mat4(1.0f), pos),
            glm::vec3(range, 1.0f, range)
        );
        glUniformMatrix4fv(circleModelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glm::vec4 color;
        switch (obj->unitStats.faction) {
            case 1:  color = glm::vec4(0.0f, 1.0f, 0.2f, 0.9f); break; // green  — player
            case 2:  color = glm::vec4(1.0f, 0.2f, 0.0f, 0.9f); break; // red    — enemy
            default: color = glm::vec4(1.0f, 1.0f, 1.0f, 0.9f); break; // white  — neutral
        }
        glUniform4fv(circleColorLoc, 1, glm::value_ptr(color));

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

// ===========================
// GAME LOOP
// ===========================

void processMovement() {
    auto& input = gps::InputManager::Instance();

    if (g_guiManager && g_guiManager->WantsKeyboardInput()) return;


    // Camera movement
    if (input.IsKeyPressed(GLFW_KEY_W)) {
        myCamera.move(gps::MOVE_UP, cameraSpeed * deltaTime);
    }
    if (input.IsKeyPressed(GLFW_KEY_S)) {
        myCamera.move(gps::MOVE_DOWN, cameraSpeed * deltaTime);
    }
    if (input.IsKeyPressed(GLFW_KEY_A)) {
        myCamera.move(gps::MOVE_LEFT, cameraSpeed * deltaTime);
    }
    if (input.IsKeyPressed(GLFW_KEY_D)) {
        myCamera.move(gps::MOVE_RIGHT, cameraSpeed * deltaTime);
    }

    if (input.IsKeyJustPressed(GLFW_KEY_F1)) {
        if (g_guiManager) {
            g_guiManager->SetDebugPanelVisible(!g_guiManager->IsDebugPanelVisible());
        }
    }

    /*
    if (input.IsKeyJustPressed(GLFW_KEY_F2)) {
        if (g_guiManager) {
            g_guiManager->SetControlPanelVisible(!g_guiManager->IsControlPanelVisible());
        }
    }
*/
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

        // Model matrix
        glm::mat4 modelMatrix = obj->GetTransform().GetModelMatrix();
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));

        // Normal matrix
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
            if (g_tileManager.GetGridBounds(gridMin, gridMax)) {
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

    // For orthographic projection, all rays travel parallel to the camera's
    // forward direction. We use the camera's front direction directly.
    glm::vec3 rayDirection = myCamera.getCameraFrontDirection();

    // Find where this ray intersects the ground plane at y = -60
    // Ray equation: P = origin + t * direction
    // We want P.y = -60, so: nearPointWorld.y + t * rayDirection.y = -60
    float groundY = -60.0f;
    float t = (groundY - nearPointWorld.y) / rayDirection.y;

    // Calculate the final intersection point
    glm::vec3 intersectionPoint = glm::vec3(nearPointWorld) + t * rayDirection;

    // Optional: Clamp to map boundaries
    // intersectionPoint.x = glm::clamp(intersectionPoint.x, MAP_MIN_X, MAP_MAX_X);
    // intersectionPoint.z = glm::clamp(intersectionPoint.z, MAP_MIN_Z, MAP_MAX_Z);

    return intersectionPoint;
}

// ===========================
// MAIN
// ===========================

int main(int argc, const char* argv[]) {
    std::cout << "🎮 Starting OpenGL Project..." << std::endl;

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

    // Init NEW SYSTEMS
    initSceneManager();
    initSelectionSystem();
    initCollisionSystem();
    initCombatSystem();
    initGui();

    g_editorState = new gps::EditorState();
    g_guiManager->BindEditorState(g_editorState);

    std::cout << "✅ Initialization complete!" << std::endl;
    std::cout << "\n🎮 CONTROLS:" << std::endl;
    std::cout << "  WASD - Move camera" << std::endl;
    std::cout << "  B - Spawn single troop" << std::endl;
    std::cout << "  N - Spawn troop formation (10 troops)" << std::endl;
    std::cout << "  T - Select all troops" << std::endl;
    std::cout << "  C - Clear selection" << std::endl;
    std::cout << "  Left Mouse - Box selection (hold SHIFT to add)" << std::endl;
    std::cout << "  Right Mouse - Move selected troops" << std::endl;
    std::cout << "  Place mode: use GUI 'Place ...' button, then Left Mouse to place" << std::endl;
    std::cout << "  Right Mouse or X - Cancel place mode" << std::endl;
    std::cout << "  Scroll - Zoom" << std::endl;
    std::cout << "  ESC - Exit\n" << std::endl;

    // Game loop
    lastFrameTime = glfwGetTime();

    while (!glfwWindowShouldClose(glWindow)) {
        // Update
        gps::InputManager::Instance().Update();
        updateDeltaTime();

        processMovement();

        // Update scene
        if (g_scene) {
            g_scene->Update(deltaTime);
        }

        // ========== PLAY MODE ONLY SYSTEMS ==========
        if (!g_editorState || g_editorState->IsPlayMode()) {

        // Resource production (with tile bonus when extractor is parked on a matching tile)
        if (g_scene) {
            for (auto* obj : g_scene->GetObjectsRaw()) {
                if (!obj->IsActive()) continue;
                const gps::UnitStats& s = obj->unitStats;
                if (s.productionRate > 0.0f && !s.resourceType.empty() && s.resourceType != "none") {
                    float bonus = 1.0f;
                    int gx, gz;
                    g_tileManager.WorldToGrid(obj->GetTransform().GetPosition(), gx, gz);
                    if (gps::Tile* t = g_tileManager.GetTile(gx, gz)) {
                        const gps::TileEffect& e = gps::GetTileEffect(t->type);
                        if (e.resource == s.resourceType) bonus = e.productionBonus;
                    }
                    gps::ResourceManager::Instance().Deposit(s.resourceType, s.productionRate * bonus * deltaTime);
                }
            }
        }

        // Combat update
        if (g_combatSystem) {
            g_combatSystem->Update(deltaTime);
            for (int deadID : g_combatSystem->GetDeadIDs()) {
                if (g_selectionSystem) g_selectionSystem->RemoveFromSelection(deadID);
                if (g_scene) g_scene->DestroyObject(deadID);
            }
            g_combatSystem->ClearDeadIDs();
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

                // Circle equation: P = Center + R * (cos(a), 0, sin(a))
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
        bool hasGridBounds = g_tileManager.GetGridBounds(gridBoundsMin, gridBoundsMax);

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

            // Land tiles block ships: revert and halt at the boundary.
            if (!obj->projectileData.isProjectile) {
                int gx, gz;
                g_tileManager.WorldToGrid(newPos, gx, gz);
                if (gps::Tile* t = g_tileManager.GetTile(gx, gz)) {
                    if (t->type == gps::TileType::Land) {
                        obj->GetTransform().SetPosition(oldPos);
                        obj->movement.isMoving = false;
                        obj->UpdateWorldBounds();
                        continue;
                    }
                }
            }

            // Sphere-sphere collisions: slide along the contact instead of locking up.
            if (g_collisionSystem && !obj->projectileData.isProjectile) {
                gps::SceneObject* other = g_collisionSystem->GetCollidingObject(obj);
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

        // Projectile arrival: apply damage and destroy stopped projectiles
        {
            std::vector<int> toDestroy;
            std::vector<int> killed;
            for (const auto& objPtr : g_scene->GetObjects()) {
                gps::SceneObject* obj = objPtr.get();
                if (!obj->IsActive() || !obj->projectileData.isProjectile || obj->movement.isMoving) continue;

                auto applyDamage = [&](gps::SceneObject* victim, int dmg) {
                    if (!victim || !victim->IsActive() || !victim->unitStats.isAlive) return;
                    victim->unitStats.health -= dmg;
                    if (victim->unitStats.health <= 0) {
                        victim->unitStats.health = 0;
                        victim->unitStats.isAlive = false;
                        victim->SetActive(false);
                        killed.push_back(victim->GetID());
                    } else if (victim->unitStats.targetID == -1) {
                        // Retaliation: assign projectile owner so Defensive units fight back
                        victim->unitStats.targetID = obj->projectileData.ownerID;
                    }
                };

                const int dmg = (int)obj->projectileData.damage;
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
                        applyDamage(victim, dmg);
                    }
                } else {
                    applyDamage(g_scene->GetObjectByID(obj->projectileData.targetID), dmg);
                }
                toDestroy.push_back(obj->GetID());
            }
            for (int id : killed) {
                if (g_selectionSystem) g_selectionSystem->RemoveFromSelection(id);
                g_scene->DestroyObject(id);
            }
            for (int id : toDestroy) g_scene->DestroyObject(id);
        }

        } // end play-mode-only systems

    myCustomShader.useShaderProgram();
    
    glm::mat4 view = myCamera.getViewMatrix();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // ✅ UPDATE point light în eye space
    glm::vec3 pointLightPosEye = glm::vec3(view * glm::vec4(lightDir, 1.0f));
    glUniform3fv(lightPosEyeLoc, 1, glm::value_ptr(pointLightPosEye));

    // ✅ UPDATE directional light în eye space
    glm::vec3 dirLightEye = glm::inverseTranspose(glm::mat3(view)) * dirLightWorld;
    glUniform3fv(dirLightDirEyeLoc, 1, glm::value_ptr(dirLightEye));

    // ✅ UPDATE camera position în eye space
    glm::mat4 invView = glm::inverse(view);
    glm::vec3 cameraPosWorld = glm::vec3(invView[3]);
    glm::vec3 cameraPosEye = glm::vec3(view * glm::vec4(cameraPosWorld, 1.0f));
    glUniform3fv(viewPosEyeLoc, 1, glm::value_ptr(cameraPosEye));


    if (g_guiManager) {
        g_guiManager->SetDeltaTime(deltaTime);
        g_guiManager->SetCameraPosition(myCamera.getCameraPosition());
        g_guiManager->SetZoomFactor(zoomFactor);
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
        glfwPollEvents();
        glfwSwapBuffers(glWindow);
    }

    // Cleanup
    delete g_editorState;
    delete g_guiManager;
    delete g_scene;
    delete g_sceneManager;
    delete g_selectionSystem;
    delete g_collisionSystem;
    delete g_combatSystem;

    glfwDestroyWindow(glWindow);
    glfwTerminate();

    std::cout << "👋 Goodbye!" << std::endl;

    return 0;
}