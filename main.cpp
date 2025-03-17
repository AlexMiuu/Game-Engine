//
//  main.cpp
//  OpenGL Advances Lighting
//
//  Created by CGIS on 28/11/16.
//  Modified to add directional lighting + fix point light
//

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
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.hpp"
#include "Model3D.hpp"
#include "Camera.hpp"
#include "SkyBox.hpp"
#include <iostream>
#include <vector>
#include <limits>
#include <unordered_set>

#include "RayCaster.hpp"
#include "CameraIso.hpp"

int glWindowWidth = 1920;
int glWindowHeight = 1080;
int retina_width, retina_height;
GLFWwindow* glWindow = NULL;

glm::mat4 model;
GLuint modelLoc;
glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;
glm::mat3 normalMatrix;
GLuint normalMatrixLoc;

// This variable will store the WORLD-Space position of our point light.
glm::vec3 lightDir = glm::vec3(0.0f, 1.0f, 1.0f);


GLuint lightPosEyeLoc;

// Point-light color
glm::vec3 lightColor;
GLuint lightColorLoc;

// Directional light
float x = 200.0f;
float y = 150.0f;;
float z = 250.0f;

float intesity_x = 0.64f;
float intensity_y = 0.55f;
float intensity_z = 0.459f;
glm::vec3 dirLightWorld = glm::vec3(x, y, z);
glm::vec3 dirLightColorVal = glm::vec3(0.647f, 0.565f, 0.459f);
GLint dirLightDirEyeLoc;
GLint dirLightColorLoc;
GLint objectIDLoc;
// Models
gps::Model3D sceneModel;
gps::Model3D orcModel;
gps::Model3D obiecte;
gps::Model3D dragon;
gps::Model3D rightWingModel;
gps::Model3D leftWingModel;
gps::Model3D dragon2;


// Camera
gps::Camera myCamera(
    glm::vec3(0.0f, 500, -50)  // Position camera for isometric view
);
float cameraSpeed = 2.0f;
//CameraISO myCamera(-150.0f, 150.0f, -150.0f, 150.0f, 0.1f, 1000.0f);

bool pressedKeys[1024];
float angleY = 0.0f;

GLuint depthMapFBO;
GLuint depthMap;
const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;
glm::mat4 lightSpaceMatrix;

// Shaders
gps::Shader myCustomShader;
gps::Shader depthShader;



// Skybox geometry + texture
unsigned int skyboxVAO, skyboxVBO;
unsigned int skyboxTexture;

// Skybox shader
gps::Shader skyboxShader;
gps::SkyBox daySkyBox;
gps::SkyBox nightSkyBox;

bool dayCycle = true;

int cursorCenterY = glWindowHeight / 2;
int cursorCenterX = glWindowWidth / 2;


GLint isSelectedLoc;
GLint highlightColorLoc;


std::unordered_set<int> selectedTroopIDs; // Stores IDs of selected troops
int nextTroopID = 100; // Starting ID for new troops
glm::vec3 troopSpawnPos = glm::vec3(100.0f, -60.0f, -90.0f); // Default spawn position

static bool spawnEnabled = false;

struct SceneObject {
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

    glm::vec3 scale = glm::vec3(1.0f); // Added scaling factor
};

std::vector<SceneObject> g_sceneObjects;

// Add this global variable near the top with other globals
float zoomFactor = 1.0f; // 1.0 = normal zoom, < 1.0 = zoomed in, > 1.0 = zoomed out

void computeLocalBoundingSphere(gps::Model3D& model, glm::vec3& outCenter, float& outRadius)
{
    glm::vec3 minPos(FLT_MAX);
    glm::vec3 maxPos(-FLT_MAX);

    for (auto& mesh : model.getMeshes()) {
        for (auto& vertex : mesh.vertices) {
            minPos = glm::min(minPos, vertex.Position);
            maxPos = glm::max(maxPos, vertex.Position);
        }
    }

    outCenter = 0.5f * (minPos + maxPos);
    outRadius = glm::length(maxPos - minPos) * 0.5f;
}

void updateWorldBounds(SceneObject& obj)
{
    glm::vec4 c = obj.modelMatrix * glm::vec4(obj.localCenter, 1.0f);
    obj.worldCenter = glm::vec3(c);

    float scaleX = glm::length(glm::vec3(obj.modelMatrix[0]));
    float scaleY = glm::length(glm::vec3(obj.modelMatrix[1]));
    float scaleZ = glm::length(glm::vec3(obj.modelMatrix[2]));
    float maxScale = std::max(scaleX, std::max(scaleY, scaleZ));

    obj.worldRadius = obj.localRadius * maxScale;
}

GLenum glCheckError_(const char* file, int line) {
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        std::string error;
        switch (errorCode)
        {
        case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
        case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
        case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
        case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    fprintf(stdout, "window resized to width: %d , and height: %d\n", width, height);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_K && action == GLFW_PRESS)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (key == GLFW_KEY_L && action == GLFW_PRESS)
    {
        glfwSetCursorPos(window, cursorCenterX, cursorCenterY);

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        glfwSetCursorPos(window, cursorCenterX, cursorCenterY);
    }

    if (key == GLFW_KEY_O && action == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    if (key == GLFW_KEY_P && action == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    if (key == GLFW_KEY_N && action == GLFW_PRESS)
    {
        dayCycle = false;
        dirLightColorVal = glm::vec3(0.1f, 0.1f, 0.1f);
        glUniform3fv(dirLightColorLoc, 1, glm::value_ptr(dirLightColorVal));
    }
    if (key == GLFW_KEY_M && action == GLFW_PRESS)
    {
        dayCycle = true;
        dirLightColorVal = glm::vec3(0.647f, 0.565f, 0.459f);
        glUniform3fv(dirLightColorLoc, 1, glm::value_ptr(dirLightColorVal));
    }

    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        spawnEnabled = true;
    }
    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    {
        spawnEnabled = false;
    }




    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)
            pressedKeys[key] = true;
        else if (action == GLFW_RELEASE)
            pressedKeys[key] = false;
    }
}



void initSkybox() {
    std::vector<const GLchar*> faces;
    faces.push_back("textures/skybox/rt.tga");
    faces.push_back("textures/skybox/lf.tga");
    faces.push_back("textures/skybox/up.tga");
    faces.push_back("textures/skybox/dn.tga");
    faces.push_back("textures/skybox/bk.tga");
    faces.push_back("textures/skybox/ft.tga");
    daySkyBox.Load(faces);
}

void initSkybox2() {
    std::vector<const GLchar*> faces;
    faces.push_back("textures/skybox/rt.png");
    faces.push_back("textures/skybox/lf.png");
    faces.push_back("textures/skybox/up.png");
    faces.push_back("textures/skybox/dn.png");
    faces.push_back("textures/skybox/ft.png");
    faces.push_back("textures/skybox/bk.png");
    nightSkyBox.Load(faces);
}



// We will also need to update the point-light position each time we move the camera.
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    static bool firstMouse = true;
    static float lastX = 0.0f;
    static float lastY = 0.0f;
    static float yaw = -90.0f;
    static float pitch = 0.0f;

    float sensitivity = 0.1f;

    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    lastX = (float)xpos;
    lastY = (float)ypos;

    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)  pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // Rotate the camera, then recalc the view matrix
   // myCamera.rotate(pitch, yaw);
    view = myCamera.getViewMatrix();

    // Upload the updated view and normal matrix
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // Update camera position in EYE space for the shader
    glm::mat4 invView = glm::inverse(view);
    glm::vec3 cameraPosWorld = glm::vec3(invView[3]);
    glm::vec3 cameraPosEye = glm::vec3(view * glm::vec4(cameraPosWorld, 1.0f));
    GLint viewPosEyeLoc = glGetUniformLocation(myCustomShader.shaderProgram, "viewPosEye");
    glUniform3fv(viewPosEyeLoc, 1, glm::value_ptr(cameraPosEye));

    // If needed, also update your point-light / directional-light 
    // in eye space here (unchanged from your prior code).
}

void processMovement() {
    bool cameraMoved = false;

    if (pressedKeys[GLFW_KEY_W]) {
        myCamera.move(gps::MOVE_UP, cameraSpeed);
    }
    if (pressedKeys[GLFW_KEY_S]) {
        myCamera.move(gps::MOVE_DOWN, cameraSpeed);
    }
    if (pressedKeys[GLFW_KEY_A]) {
        myCamera.move(gps::MOVE_LEFT, cameraSpeed);
    }
    if (pressedKeys[GLFW_KEY_D]) {
        myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
    }


    if (pressedKeys[GLFW_KEY_UP]) {
        y += 1.0f;
    }

    if (pressedKeys[GLFW_KEY_DOWN]) {
        y -= 1.0f;
    }


    if (pressedKeys[GLFW_KEY_LEFT]) {
        z += 1.0f;
    }

    if (pressedKeys[GLFW_KEY_RIGHT]) {
        z -= 1.0f;
    }

    if (cameraMoved) {
        // Recalculate view matrix
        view = myCamera.getViewMatrix();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        // Update normal matrix if needed
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

        // Update camera pos in EYE space
        glm::mat4 invView = glm::inverse(view);
        glm::vec3 cameraPosWorld = glm::vec3(invView[3]);
        glm::vec3 cameraPosEye = glm::vec3(view * glm::vec4(cameraPosWorld, 1.0f));
        GLint viewPosEyeLoc = glGetUniformLocation(myCustomShader.shaderProgram, "viewPosEye");
        glUniform3fv(viewPosEyeLoc, 1, glm::value_ptr(cameraPosEye));

    }
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    // yoffset positive = scroll up (zoom in)
    // yoffset negative = scroll down (zoom out)

    // Adjust for smoother zooming with exponential scaling
    float zoomSpeed = 0.1f;

    if (yoffset > 0) {
        // Zoom in (decrease factor) - multiply for smoother zoom in
        zoomFactor *= (1.0f - zoomSpeed);
    }
    else {
        // Zoom out (increase factor) - multiply for smoother zoom out
        zoomFactor *= (1.0f + zoomSpeed);
    }

    // Clamp to reasonable values with better limits for this scene
    if (zoomFactor < 0.2f) zoomFactor = 0.2f;  // Maximum zoom in (closer)
    if (zoomFactor > 3.0f) zoomFactor = 3.0f;  // Maximum zoom out (farther)

    // Update the projection matrix based on the zoom factor
    float aspectRatio = float(retina_width) / float(retina_height);
    float orthoSize = 150.0f * zoomFactor;  // Base orthoSize * zoom factor

    projection = glm::ortho(
        -orthoSize * aspectRatio, orthoSize * aspectRatio,
        -orthoSize, orthoSize,
        -1000.0f, 1000.0f
    );

    // Make sure to update the projection for ALL shaders that need it
    // Main shader
    myCustomShader.useShaderProgram();
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Skybox shader
    skyboxShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // Switch back to main shader
    myCustomShader.useShaderProgram();

    std::cout << "Zoom factor: " << zoomFactor << std::endl;
}


bool initOpenGLWindow()
{
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: could not start GLFW3\n");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight,
        "OpenGL Shader Example", NULL, NULL);
    if (!glWindow) {
        fprintf(stderr, "ERROR: could not open window with GLFW3\n");
        glfwTerminate();
        return false;
    }

    glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
    glfwSetKeyCallback(glWindow, keyboardCallback);
    glfwSetCursorPosCallback(glWindow, mouseCallback);
    glfwSetScrollCallback(glWindow, scrollCallback);

    // Enable cursor for troop selection
    glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    glfwMakeContextCurrent(glWindow);
    glfwSwapInterval(1);

#if not defined (__APPLE__)
    glewExperimental = GL_TRUE;
    glewInit();
#endif

    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported %s\n", version);

    glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

    return true;
}

void initOpenGLState()
{
    glClearColor(0.3f, 0.3f, 0.3f, 1.0);
    glViewport(0, 0, retina_width, retina_height);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_FRAMEBUFFER_SRGB);
}

void initObjects() {
    sceneModel.LoadModel("objects/teren/teren.obj", "textures/");
    orcModel.LoadModel("objects/ORC/ORC.obj", "textures/");
    obiecte.LoadModel("objects/restobiecte/restobiecte.obj", "textures/");
    dragon.LoadModel("objects/dragon/dragon.obj", "textures/");
    leftWingModel.LoadModel("objects/LEFTWING/LEFTWING.obj", "textures/");
    rightWingModel.LoadModel("objects/RIGHTWING/RIGHTWING.obj", "textures/");
    dragon2.LoadModel("objects/dragon2/dragon2.obj", "textures/");
    // Scene:
    {
        SceneObject obj;
        obj.id = 1;
        obj.modelPtr = &sceneModel;
        obj.modelMatrix = glm::mat4(1.0f);
        computeLocalBoundingSphere(sceneModel, obj.localCenter, obj.localRadius);
        updateWorldBounds(obj);
        g_sceneObjects.push_back(obj);
    }

    // Orc:
    {
        SceneObject obj;
        obj.id = 2;
        obj.modelPtr = &orcModel;
        glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(100.0f, -60.0f, -90.0f));
        glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(3.5f));
        obj.modelMatrix = T * S;
        obj.scale = glm::vec3(3.5f); // Set scaling factor
        computeLocalBoundingSphere(orcModel, obj.localCenter, obj.localRadius);
        updateWorldBounds(obj);
        g_sceneObjects.push_back(obj);
    }


    //dragon 
    {
        SceneObject obj;
        obj.id = 4;
        obj.modelPtr = &dragon;
        glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 10.0f, -90.0f));
        // glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
        //  glm::mat4 R = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        obj.modelMatrix = T;
        computeLocalBoundingSphere(dragon, obj.localCenter, obj.localRadius);
        updateWorldBounds(obj);
        g_sceneObjects.push_back(obj);

    }

    //DRAGON 2
    {
        SceneObject obj;
        obj.id = 5;
        obj.modelPtr = &dragon2;
        glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 15.0f, 90.0f));
        // glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
        glm::mat4 R = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        obj.modelMatrix = T * R;
        computeLocalBoundingSphere(dragon2, obj.localCenter, obj.localRadius);
        updateWorldBounds(obj);
        g_sceneObjects.push_back(obj);

    }
    // left and right wings
    // 

    {
        SceneObject leftWing;
        leftWing.id = 10; // Unique ID for left wing
        leftWing.modelPtr = &leftWingModel;

        // Initial Position: Relative to the dragon's initial position
        // Adjust the offset based on your models' alignment

        glm::mat4 T_left = glm::translate(glm::mat4(1.0f), glm::vec3(-55.0f, 10.f, -55.0f));
        glm::mat4 S_left = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f)); // Adjust scale if necessary
        leftWing.modelMatrix = T_left * S_left;

        computeLocalBoundingSphere(leftWingModel, leftWing.localCenter, leftWing.localRadius);
        updateWorldBounds(leftWing);

        g_sceneObjects.push_back(leftWing);


        SceneObject rightWing;
        rightWing.id = 11; // Unique ID for right wing
        rightWing.modelPtr = &rightWingModel;

        glm::vec3 rightWingOffset = glm::vec3(1.5f, 0.0f, 0.0f); // Adjust as needed

        glm::mat4 T_right = glm::translate(glm::mat4(1.0f), glm::vec3(-55.0f, 10.0f, -55.0f));
        glm::mat4 S_right = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f)); // Adjust scale if necessary
        glm::mat4 R_right = glm::mat4(1.0f); // Initial rotation if needed

        rightWing.modelMatrix = T_right * S_right;

        computeLocalBoundingSphere(rightWingModel, rightWing.localCenter, rightWing.localRadius);
        updateWorldBounds(rightWing);

        g_sceneObjects.push_back(rightWing);
    }


    //rest of the objects
    {
        SceneObject obj;
        obj.id = 3;
        obj.modelPtr = &obiecte;
        obj.modelMatrix = glm::mat4(1.0f);
        computeLocalBoundingSphere(obiecte, obj.localCenter, obj.localRadius);
        updateWorldBounds(obj);
        g_sceneObjects.push_back(obj);
    }





}

void initShaders() {
    depthShader.loadShader("shaders/depth.vert", "shaders/depth.frag");
    depthShader.useShaderProgram();

    myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
    myCustomShader.useShaderProgram();

    skyboxShader.loadShader("shaders/skybox.vert", "shaders/skybox.frag");
    skyboxShader.useShaderProgram();
}

// Updated initUniforms to properly set point light & directional light
void initUniforms() {
    myCustomShader.useShaderProgram();

    // Model
    model = glm::mat4(1.0f);
    modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // View
    view = myCamera.getViewMatrix();
    viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // Normal Matrix
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // Projection
    float aspectRatio = float(retina_width) / float(retina_height);
    float orthoSize = 150.0f * zoomFactor;
    projection = glm::ortho(
        -orthoSize * aspectRatio, orthoSize * aspectRatio,
        -orthoSize, orthoSize,
        -1000.0f, 1000.0f
    );

    projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    //------------------------------------------------
    // POINT LIGHT (treated as "lightPosEye" in shader)
    //------------------------------------------------
    // We'll keep using `lightDir` as the point-light *world* position.
    lightPosEyeLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightPosEye");
    // Transform it into EYE space using the view matrix
    glm::vec3 pointLightPosEye = glm::vec3(view * glm::vec4(lightDir, 1.0f));
    glUniform3fv(lightPosEyeLoc, 1, glm::value_ptr(pointLightPosEye));

    // Point-light color
    lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    //------------------------------------------------
    // DIRECTIONAL LIGHT
    //------------------------------------------------
    dirLightDirEyeLoc = glGetUniformLocation(myCustomShader.shaderProgram, "dirLightDirEye");
    dirLightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "dirLightColor");
    glm::vec3 dirLightEye = glm::inverseTranspose(glm::mat3(view)) * dirLightWorld;
    glUniform3fv(dirLightDirEyeLoc, 1, glm::value_ptr(dirLightEye));
    glUniform3fv(dirLightColorLoc, 1, glm::value_ptr(dirLightColorVal));

    //------------------------------------------------
    // CAMERA position in eye space (for specular)
    //------------------------------------------------
    glm::mat4 invView = glm::inverse(view);
    glm::vec3 cameraPosWorld = glm::vec3(invView[3]); // translation part of invView
    glm::vec3 cameraPosEye = glm::vec3(view * glm::vec4(cameraPosWorld, 1.0f));

    GLint viewPosEyeLoc = glGetUniformLocation(myCustomShader.shaderProgram, "viewPosEye");
    glUniform3fv(viewPosEyeLoc, 1, glm::value_ptr(cameraPosEye));


    isSelectedLoc = glGetUniformLocation(myCustomShader.shaderProgram, "isSelected");
    highlightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "highlightColor");

     objectIDLoc = glGetUniformLocation(myCustomShader.shaderProgram, "objectID");
}

void initShadowMapping() {
    glGenFramebuffers(1, &depthMapFBO);
    glGenTextures(1, &depthMap);

    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D,
        0,
        GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT,
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D,
        depthMap,
        0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix() {
    // Make the orthographic bounds bigger so we capture the entire scene.
    // For instance, if your terrain is ~300x300 or more, try ±150 or ±200.
    glm::mat4 lightProjection = glm::ortho(
        -150.0f,  // left
        150.0f,  // right
        -150.0f,  // bottom
        150.0f,  // top
        0.1f,   // near plane
        600.0f    // far plane (big enough to enclose your terrain's height)
    );

    // Pick a higher, more "top-down" position for the directional light
    // so it casts shadows across the whole terrain.
    glm::mat4 lightView = glm::lookAt(
        glm::vec3(z, y, z), // new light position above & offset
        glm::vec3(0.0f, 0.0f, 0.0f),  // looking toward the center
        glm::vec3(0.0f, 1.0f, 0.0f)   // up axis
    );

    return lightProjection * lightView;
}


void renderScene()
{
    glViewport(0, 0, retina_width, retina_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 1) Render the scene objects with your normal shader
    myCustomShader.useShaderProgram();

    // If you use shadow mapping:
    glm::mat4 lightSpaceMatrix = computeLightSpaceTrMatrix();
    glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceMatrix"),
        1,
        GL_FALSE,
        glm::value_ptr(lightSpaceMatrix));

    // Bind shadow map texture
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);

    glDepthFunc(GL_LEQUAL);   // skybox passes if depth >= existing
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);


    if (dayCycle)
        daySkyBox.Draw(skyboxShader, view, projection);
    else
        nightSkyBox.Draw(skyboxShader, view, projection);


    glEnable(GL_CULL_FACE);

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);


    // Draw all objects in the scene
    for (auto& obj : g_sceneObjects) {
        // Set object ID for the shader
        glUniform1i(objectIDLoc, obj.id);

        bool isTroopSelected = selectedTroopIDs.count(obj.id) > 0;
        glUniform1i(isSelectedLoc, isTroopSelected);
        glUniform3fv(highlightColorLoc, 1, glm::value_ptr(glm::vec3(1.0, 1.0, 0.0))); // Yellow highlight

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(obj.modelMatrix));
        glm::mat3 nm = glm::mat3(glm::inverseTranspose(view * obj.modelMatrix));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
        obj.modelPtr->Draw(myCustomShader);
    }
}


void renderSceneFromLight() {
    lightSpaceMatrix = computeLightSpaceTrMatrix();
    depthShader.useShaderProgram();

    glUniformMatrix4fv(glGetUniformLocation(depthShader.shaderProgram, "lightSpaceMatrix"),
        1,
        GL_FALSE,
        glm::value_ptr(lightSpaceMatrix));

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR: Framebuffer not complete!" << std::endl;
    }
    glClear(GL_DEPTH_BUFFER_BIT);

    for (auto& obj : g_sceneObjects) {
        glUniformMatrix4fv(glGetUniformLocation(depthShader.shaderProgram, "model"),
            1,
            GL_FALSE,
            glm::value_ptr(obj.modelMatrix));
        obj.modelPtr->Draw(depthShader);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void cleanup() {
    glfwDestroyWindow(glWindow);
    glfwTerminate();
}

// Helper that returns camera world pos from the view matrix:
static glm::vec3 getCameraPosFromViewMatrix(const glm::mat4& viewMat)
{
    glm::mat4 invView = glm::inverse(viewMat);
    return glm::vec3(invView[3]);
}

SceneObject* getSceneObjectById(int id) {
    for (auto& obj : g_sceneObjects) {
        if (obj.id == id) {
            return &obj;
        }
    }
    return nullptr;
}

static bool orcSelected = false;

void spawnTroop(glm::vec3 position) {
    // Get current camera position before any changes
    glm::vec3 cameraPosOriginal = getCameraPosFromViewMatrix(view);

    SceneObject newTroop;
    newTroop.id = nextTroopID++;
    newTroop.modelPtr = &orcModel;

    // Make sure Y position is at the right height to avoid clipping with terrain
    position.y = -60.0f;

    // Create model matrix with translation and scale
    glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(3.5f));
    newTroop.modelMatrix = T * S;
    newTroop.scale = glm::vec3(3.5f);

    // Calculate bounding sphere
    computeLocalBoundingSphere(*newTroop.modelPtr, newTroop.localCenter, newTroop.localRadius);

    // Add to scene objects and update bounds
    g_sceneObjects.push_back(newTroop);
    updateWorldBounds(g_sceneObjects.back());

    // Make sure model matrix for base drawing is identity
    model = glm::mat4(1.0f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Update ALL shader uniforms completely
    myCustomShader.useShaderProgram();

    // Recompute and update view matrix
    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // Update normal matrix
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // Update projection matrix
    float aspectRatio = float(retina_width) / float(retina_height);
    float orthoSize = 150.0f * zoomFactor;
    projection = glm::ortho(
        -orthoSize * aspectRatio, orthoSize * aspectRatio,
        -orthoSize, orthoSize,
        -1000.0f, 1000.0f
    );
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Update lighting uniforms
    glm::vec3 pointLightPosEye = glm::vec3(view * glm::vec4(lightDir, 1.0f));
    glUniform3fv(lightPosEyeLoc, 1, glm::value_ptr(pointLightPosEye));

    glm::vec3 dirLightEye = glm::inverseTranspose(glm::mat3(view)) * dirLightWorld;
    glUniform3fv(dirLightDirEyeLoc, 1, glm::value_ptr(dirLightEye));

    // Update camera position in EYE space
    glm::mat4 invView = glm::inverse(view);
    glm::vec3 cameraPosWorld = glm::vec3(invView[3]);
    glm::vec3 cameraPosEye = glm::vec3(view * glm::vec4(cameraPosWorld, 1.0f));
    GLint viewPosEyeLoc = glGetUniformLocation(myCustomShader.shaderProgram, "viewPosEye");
    glUniform3fv(viewPosEyeLoc, 1, glm::value_ptr(cameraPosEye));

    std::cout << "Spawned new troop with ID: " << (nextTroopID - 1) << " at position: "
        << position.x << ", " << position.y << ", " << position.z << std::endl;
}





void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        // Get cursor position and create ray
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        // Convert to NDC and create ray
        float ndcX = (2.0f * mouseX) / retina_width - 1.0f;
        float ndcY = 1.0f - (2.0f * mouseY) / retina_height;

        // In orthographic projection, rays are parallel
        // Ray direction is simply the view direction transformed to world space
        glm::vec3 rayDir = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));
        glm::vec3 worldRayDir = glm::vec3(glm::inverse(view) * glm::vec4(rayDir, 0.0f));
        worldRayDir = glm::normalize(worldRayDir);

        // Ray origin should start at the mouse position in world space
        glm::vec4 rayOriginClip = glm::vec4(ndcX, ndcY, -1.0f, 1.0f); // -1 = near plane
        glm::vec4 rayOriginEye = glm::inverse(projection) * rayOriginClip;
        rayOriginEye.w = 1.0f;

        glm::vec4 rayOriginWorld = glm::inverse(view) * rayOriginEye;
        glm::vec3 rayOrigin = glm::vec3(rayOriginWorld) / rayOriginWorld.w;

        gps::Ray ray;
        ray.origin = rayOrigin;
        ray.direction = worldRayDir;

        // Debug message for ray creation
        std::cout << "Ray created at (" << mouseX << ", " << mouseY << ") screen coords" << std::endl;
        std::cout << "Ray origin: (" << ray.origin.x << ", " << ray.origin.y << ", " << ray.origin.z << ")" << std::endl;
        std::cout << "Ray direction: (" << ray.direction.x << ", " << ray.direction.y << ", " << ray.direction.z << ")" << std::endl;

        // Check for troop intersection (ID == 2)
        bool troopHit = false;
        int hitTroopID = -1;
        float closestTroopDist = FLT_MAX;

        // Loop through all scene objects
        for (auto& obj : g_sceneObjects) {
            // Check if this is a troop (ID==2 or ID>=100)
            if (obj.id == 2 || obj.id >= 100) {
                float distHit;
                if (gps::RayIntersectsSphere(ray, obj.worldCenter, obj.worldRadius, distHit)) {
                    // If this is closer than any previous hit, update the hit info
                    if (distHit < closestTroopDist) {
                        closestTroopDist = distHit;
                        hitTroopID = obj.id;
                        troopHit = true;
                        std::cout << "Debug: Troop hit at distance " << distHit << ", ID: " << obj.id << std::endl;
                    }
                }
            }
        }

        // Check for ground intersection
        bool groundHit = false;
        glm::vec3 groundHitPoint;
        SceneObject* terrainObj = getSceneObjectById(1);
        if (terrainObj) {
            float outT;
            groundHit = gps::RayIntersectsModelWithMatrix(ray, *(terrainObj->modelPtr),
                terrainObj->modelMatrix, outT, groundHitPoint);
            if (groundHit) {
                std::cout << "Debug: Ground hit at (" << groundHitPoint.x << ", "
                    << groundHitPoint.y << ", " << groundHitPoint.z << ")" << std::endl;
            }
        }

        // Process clicks
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (troopHit) {
                // Select troop
                selectedTroopIDs.insert(hitTroopID);
                std::cout << "Debug: Troop ID " << hitTroopID << " selected" << std::endl;
            }
            else if (groundHit) {
                if (spawnEnabled) {
                    // Spawn troop at hit location
                    spawnTroop(groundHitPoint);
                    std::cout << "Debug: Spawned new troop at ground hit point" << std::endl;
                }
                else if (!selectedTroopIDs.empty()) {
                    // Move selected troops to the ground hit location
                    std::cout << "Debug: Moving " << selectedTroopIDs.size() << " selected troops to ground hit point" << std::endl;

                    for (int troopID : selectedTroopIDs) {
                        SceneObject* selectedTroop = getSceneObjectById(troopID);
                        if (selectedTroop) {
                            // Set up movement
                            selectedTroop->isMoving = true;
                            selectedTroop->moveStartTime = static_cast<float>(glfwGetTime());
                            selectedTroop->moveDuration = 2.0f;
                            selectedTroop->moveStartPos = glm::vec3(selectedTroop->modelMatrix[3]);
                            selectedTroop->moveEndPos = groundHitPoint;

                            // Update bounds
                            updateWorldBounds(*selectedTroop);

                            std::cout << "Debug: Troop ID " << troopID << " moving from ("
                                << selectedTroop->moveStartPos.x << ", "
                                << selectedTroop->moveStartPos.y << ", "
                                << selectedTroop->moveStartPos.z << ") to ("
                                << groundHitPoint.x << ", "
                                << groundHitPoint.y << ", "
                                << groundHitPoint.z << ")" << std::endl;
                        }
                    }
                }
            }
            else {
                std::cout << "Debug: No intersection detected" << std::endl;
            }
        }
        else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (troopHit && selectedTroopIDs.count(hitTroopID)) {
                // Deselect troop
                selectedTroopIDs.erase(hitTroopID);
                std::cout << "Debug: Troop ID " << hitTroopID << " deselected" << std::endl;
            }
        }
    }
}


void renderSkybox() {
    // 2) Draw the skybox last, using a view matrix WITHOUT the camera translation.
//    This prevents the skybox from "sliding" or "spinning around" weirdly when you move.
    glDepthFunc(GL_LEQUAL);   // skybox passes if depth >= existing
    glDepthMask(GL_FALSE);    // no depth writes from the skybox

    skyboxShader.useShaderProgram();

    // Remove the translation from the view matrix by taking only the upper-left 3x3 rotation
    glm::mat4 skyboxView = glm::mat4(glm::mat3(view));  // zeroes out translation

    // Send matrices to skybox shader
    GLuint skyboxViewLoc = glGetUniformLocation(skyboxShader.shaderProgram, "view");
    GLuint skyboxProjLoc = glGetUniformLocation(skyboxShader.shaderProgram, "projection");
    glUniformMatrix4fv(skyboxViewLoc, 1, GL_FALSE, glm::value_ptr(skyboxView));
    glUniformMatrix4fv(skyboxProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // cube for  skybox
    glDisable(GL_CULL_FACE);
    daySkyBox.Draw(skyboxShader, skyboxView, projection);
    glEnable(GL_CULL_FACE);



    // Restore your usual depth function
    // Optionally: glDepthMask(GL_TRUE);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}




static void setupMouseButtonCallback()
{
    glfwSetMouseButtonCallback(glWindow, mouseButtonCallback);
}


int main(int argc, const char* argv[]) {
    if (!initOpenGLWindow()) {
        glfwTerminate();
        return 1;
    }

    initOpenGLState();
    initObjects();
    initShaders();
    initUniforms();
    setupMouseButtonCallback();
    initShadowMapping();
    initSkybox();
    initSkybox2();


    while (!glfwWindowShouldClose(glWindow)) {
        // Store original matrices at the beginning of each frame
        glm::mat4 originalModel = model;
        glm::mat4 originalView = view;
        glm::mat4 originalProjection = projection;

        processMovement();

        // Always use identity model matrix for base transformations
        model = glm::mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        // 1) render depth from the light's POV
        glDisable(GL_CULL_FACE);
        renderSceneFromLight();
        glEnable(GL_CULL_FACE);

        // 2) render the scene from the camera's POV
        renderScene();

        // Animate moving objects
        {
            double currentTime = glfwGetTime();
            glm::mat4 dragonModelMatrix; // To store dragon's current modelMatrix

            for (auto& obj : g_sceneObjects) {
                // Only animate specific objects (not the terrain/map)
                if (obj.id == 1 || obj.id == 3) {
                    // Skip static scene objects (terrain and other static objects)
                    continue;
                }

                float animationRadius = 60.0f;
                float animationSpeed = 0.7f;

                if (obj.id == 5) {  // Dragon
                    // Compute the new position using circular motion
                    float angle = animationSpeed * (float)currentTime;
                    float x = animationRadius * cos(angle);
                    float z = animationRadius * sin(angle);
                    float y = 60.0f + 20.0f * sin(0.5f * angle);

                    glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
                    glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));

                    obj.modelMatrix = T * S;
                    obj.modelMatrix = glm::rotate(obj.modelMatrix, angle, glm::vec3(0.0f, 1.0f, 0.0f));
                    updateWorldBounds(obj);

                    dragonModelMatrix = obj.modelMatrix;
                }
                else if (obj.id == 10 || obj.id == 11) { // Wings
                    // Wing animation code (unchanged)
                    float flapSpeed = 1.0f;
                    float flapAmplitude = 2.0f;
                    float wingFlapAngle = glm::radians(flapAmplitude) * sin(currentTime * flapSpeed * 2.0f * glm::pi<float>());

                    glm::vec3 wingOffset;
                    if (obj.id == 10) { // Left Wing
                        wingOffset = glm::vec3(-55.0f, 5.0f, -55.0f);
                    }
                    else { // Right Wing
                        wingOffset = glm::vec3(-55.0f, 5.0f, -55.0f);
                        wingFlapAngle = -wingFlapAngle;
                    }

                    glm::mat4 R = glm::rotate(glm::mat4(1.0f), wingFlapAngle, glm::vec3(1.0f, 0.0f, 0.0f));
                    obj.modelMatrix = glm::translate(glm::mat4(1.0f), wingOffset) * R * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
                    updateWorldBounds(obj);
                }
                else if (obj.isMoving) { // Moving troops
                    // Movement animation code (unchanged)
                    float elapsed = static_cast<float>(currentTime) - obj.moveStartTime;
                    float alpha = elapsed / obj.moveDuration;

                    if (alpha >= 1.0f) {
                        alpha = 1.0f;
                        obj.isMoving = false;
                    }

                    glm::vec3 newPos = glm::mix(obj.moveStartPos, obj.moveEndPos, alpha);
                    glm::mat4 T = glm::translate(glm::mat4(1.0f), newPos);
                    glm::mat4 S = glm::scale(glm::mat4(1.0f), obj.scale);

                    obj.modelMatrix = T * S;
                    updateWorldBounds(obj);
                }
                // Else do nothing - don't modify other objects
            }
        }

        // Always restore matrices after each frame is complete
        model = glm::mat4(1.0f);
        view = myCamera.getViewMatrix();

        // Update projection matrix with zoom factor
        float aspectRatio = float(retina_width) / float(retina_height);
        float orthoSize = 150.0f * zoomFactor;
        projection = glm::ortho(
            -orthoSize * aspectRatio, orthoSize * aspectRatio,
            -orthoSize, orthoSize,
            -1000.0f, 1000.0f
        );

        // Update all uniforms
        myCustomShader.useShaderProgram();
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

        glfwPollEvents();
        glfwSwapBuffers(glWindow);
    }

    cleanup();
    return 0;
}

