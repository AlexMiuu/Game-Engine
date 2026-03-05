//
// main.cpp - INTEGRAT CU NOILE SISTEME
// Exemplu complet de utilizare: InputManager, Scene, SceneManager, SelectionSystem
//

#include <iostream>
#include <cmath>

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

// Uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint viewPosLoc;
GLint isSelectedLoc;  // Pentru highlight

GLint highlightColorLoc;     
GLint objectIDLoc;

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
gps::Model3D dragon;           // Dragon
gps::Model3D leftWingModel;    // Aripi
gps::Model3D rightWingModel;

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
gps::TileManager g_tileManager;

// ===========================
// NOILE SISTEME (GLOBALE)
// ===========================
gps::Scene* g_scene = nullptr;
gps::SceneManager* g_sceneManager = nullptr;
gps::SelectionSystem* g_selectionSystem = nullptr;

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

void processMovement();
void updateDeltaTime();
void renderScene(gps::Shader shader);
void renderShadowMap();
void renderSkyBox();

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

    // ESC pentru exit
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    // Forward către InputManager
    gps::InputManager::Instance().OnMouseButton(button, action, mods);

    if (!g_selectionSystem || !g_scene) return;

    glm::vec2 mousePos = gps::InputManager::Instance().GetMousePosition();

    // LEFT MOUSE - Selection
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            g_selectionSystem->StartBoxSelection(mousePos);
        }
        else if (action == GLFW_RELEASE) {
            bool shiftHeld = gps::InputManager::Instance().IsKeyPressed(GLFW_KEY_LEFT_SHIFT) ||
                gps::InputManager::Instance().IsKeyPressed(GLFW_KEY_RIGHT_SHIFT);

            g_selectionSystem->EndBoxSelection(*g_scene, myCamera, projection, shiftHeld);
        }
    }

    // RIGHT MOUSE - Movement Command
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        glm::vec3 worldPos = screenToWorld(mousePos);

        const auto& selectedIDs = g_selectionSystem->GetSelectedIDs();
        if (selectedIDs.empty()) return;

        // Calculează formație
        int numSelected = selectedIDs.size();
        int columns = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(numSelected))));
        float spacing = 15.0f;

        int idx = 0;
        for (int troopID : selectedIDs) {
            gps::SceneObject* obj = g_scene->GetObjectByID(troopID);
            if (!obj) continue;

            int row = idx / columns;
            int col = idx % columns;
            float offsetX = (col - columns / 2.0f) * spacing;
            float offsetZ = (row - numSelected / columns / 2.0f) * spacing;

            glm::vec3 formationPos = worldPos + glm::vec3(offsetX, 0.0f, offsetZ);

            // Setează movement
            
            obj->movement.isMoving = true;
            obj->movement.moveStartPos = obj->GetTransform().GetPosition();
            obj->movement.moveEndPos = formationPos;
            obj->movement.moveStartTime = glfwGetTime();
            obj->movement.moveDuration = 2.0f;
           
            idx++;
        }

        std::cout << "📍 Moving " << selectedIDs.size() << " troops to target" << std::endl;
    }
}

void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
    gps::InputManager::Instance().OnMouseMove(xpos, ypos);

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
    dragon.LoadModel("objects/dragon/dragon.obj", "textures/");
    leftWingModel.LoadModel("objects/LEFTWING/LEFTWING.obj", "textures/");
    rightWingModel.LoadModel("objects/RIGHTWING/RIGHTWING.obj", "textures/");

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
    objectIDLoc = glGetUniformLocation(myCustomShader.shaderProgram, "objectID");

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
    g_sceneManager->RegisterModel("dragon", &dragon);
    g_sceneManager->RegisterModel("leftWing", &leftWingModel);
    g_sceneManager->RegisterModel("rightWing", &rightWingModel);

    g_tileManager.Initialize(&sceneModel, g_scene);
    // Optional: genereaza si incarca un grid 3x3
    g_tileManager.GenerateAndLoadGrid(-1, 1, -1, 1);

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



// ===========================
// GAME LOOP
// ===========================

void processMovement() {
    auto& input = gps::InputManager::Instance();

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
            glUniform3fv(highlightColorLoc, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.0f))); // Yellow
        }
        // Draw
        obj->GetModel()->Draw(shader);
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
    initUniforms();
    initSkyBox();
   // initTileManager();

    // Init NEW SYSTEMS
    initSceneManager();
    initSelectionSystem();

    std::cout << "✅ Initialization complete!" << std::endl;
    std::cout << "\n🎮 CONTROLS:" << std::endl;
    std::cout << "  WASD - Move camera" << std::endl;
    std::cout << "  B - Spawn single troop" << std::endl;
    std::cout << "  N - Spawn troop formation (10 troops)" << std::endl;
    std::cout << "  T - Select all troops" << std::endl;
    std::cout << "  C - Clear selection" << std::endl;
    std::cout << "  Left Mouse - Box selection (hold SHIFT to add)" << std::endl;
    std::cout << "  Right Mouse - Move selected troops" << std::endl;
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

        // Update troops movement
        double currentTime = glfwGetTime();
        for (const auto& objPtr : g_scene->GetObjects()) {
            gps::SceneObject* obj = objPtr.get();

            if (obj->movement.isMoving) {
                float elapsed = static_cast<float>(currentTime) - obj->movement.moveStartTime;
                float alpha = elapsed / obj->movement.moveDuration;

                if (alpha >= 1.0f) {
                    alpha = 1.0f;
                    obj->movement.isMoving = false;
                }

                glm::vec3 newPos = glm::mix(obj->movement.moveStartPos, obj->movement.moveEndPos, alpha);
                obj->GetTransform().SetPosition(newPos);
                obj->UpdateWorldBounds();
            }
        }

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



        //RENDER

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

        // Render skybox
        renderSkyBox();

        myCustomShader.useShaderProgram();



        // Render selection box (UI overlay)
        if (g_selectionSystem) {
            g_selectionSystem->Render();
        }

        // Swap buffers
        glfwPollEvents();
        glfwSwapBuffers(glWindow);
    }

    // Cleanup
    delete g_scene;
    delete g_sceneManager;
    delete g_selectionSystem;

    glfwDestroyWindow(glWindow);
    glfwTerminate();

    std::cout << "👋 Goodbye!" << std::endl;

    return 0;
}