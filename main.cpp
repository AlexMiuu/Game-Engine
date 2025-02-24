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
// Previously it was named 'lightDir', but let's keep the name 'lightDir'
// to minimize changes. Think of it now as a point light's world position.
glm::vec3 lightDir = glm::vec3(0.0f, 1.0f, 1.0f);

// We no longer use lightDirLoc as a direction uniform; 
// Instead we have a new uniform for the point light in EYE space:
GLuint lightPosEyeLoc;

// Point-light color
glm::vec3 lightColor;
GLuint lightColorLoc;

// Directional light
float x=200.0f;
float y = 150.0f;;
float z=250.0f;

float intesity_x = 0.64f;
float intensity_y = 0.55f;
float intensity_z = 0.459f;
glm::vec3 dirLightWorld = glm::vec3(x,y,z);
glm::vec3 dirLightColorVal = glm::vec3(0.647f, 0.565f, 0.459f);
GLint dirLightDirEyeLoc;
GLint dirLightColorLoc;

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
    glm::vec3(0.0f, 0.0f, 2.5f),
    glm::vec3(0.0f, 0.0f, -10.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));
float cameraSpeed = 0.3f;

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

int cursorCenterY = glWindowHeight /2;
int cursorCenterX = glWindowWidth / 2;

#include "RayCaster.hpp"


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

void computeLocalBoundingSphere(gps::Model3D& model, glm::vec3& outCenter, float& outRadius)
{
    glm::vec3 minPos(FLT_MAX);
    glm::vec3 maxPos(-FLT_MAX);

    for (auto& mesh : model.getMeshes()){
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
    myCamera.rotate(pitch, yaw);
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
        myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
        cameraMoved = true;
    }
    if (pressedKeys[GLFW_KEY_S]) {
        myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        cameraMoved = true;
    }
    if (pressedKeys[GLFW_KEY_A]) {
        myCamera.move(gps::MOVE_LEFT, cameraSpeed);
        cameraMoved = true;
    }
    if (pressedKeys[GLFW_KEY_D]) {
        myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
        cameraMoved = true;
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
        glm::vec3 cameraPosEye   = glm::vec3(view * glm::vec4(cameraPosWorld, 1.0f));
        GLint viewPosEyeLoc      = glGetUniformLocation(myCustomShader.shaderProgram, "viewPosEye");
        glUniform3fv(viewPosEyeLoc, 1, glm::value_ptr(cameraPosEye));

    }
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
    glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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
        obj.modelMatrix = T*R;
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
    projection = glm::perspective(glm::radians(45.0f),
        (float)retina_width / (float)retina_height,
        0.1f, 10000.0f);
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
        600.0f    // far plane (big enough to enclose your terrain’s height)
    );

    // Pick a higher, more “top-down” position for the directional light
    // so it casts shadows across the whole terrain.
    glm::mat4 lightView = glm::lookAt(
        glm::vec3(z,y,z), // new light position above & offset
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


    if(dayCycle)
    daySkyBox.Draw(skyboxShader, view, projection);
    else
    nightSkyBox.Draw(skyboxShader, view, projection);


    glEnable(GL_CULL_FACE);

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);


    // Draw all objects in the scene
    for (auto& obj : g_sceneObjects) {
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

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        // Build a ray from screen coords
        glm::vec3 cameraPosWorld = getCameraPosFromViewMatrix(view);
        gps::Ray ray = gps::computeRay(
            float(mouseX),
            float(mouseY),
            float(glWindowWidth),
            float(glWindowHeight),
            view,
            projection,
            cameraPosWorld
        );

        // See if user clicked an object
        float closestDist = FLT_MAX;
        int pickedID = -1;
        for (auto& obj : g_sceneObjects) {
            float distHit;
            if (gps::RayIntersectsSphere(ray, obj.worldCenter, obj.worldRadius, distHit)) {
                if (distHit < closestDist) {
                    closestDist = distHit;
                    pickedID = obj.id;
                }
            }
        }

        // If first click is on the orc
        if (!orcSelected && pickedID == 2) {
            orcSelected = true;
            std::cout << "Orc selected. Next click on terrain to place it.\n";
        }
        // If orc is already selected, second click means "place it"
        else if (orcSelected) {
            SceneObject* terrainObj = getSceneObjectById(1); // terrain
            if (terrainObj) {
                float outT;
                glm::vec3 outPoint;
                bool hit = gps::RayIntersectsModelWithMatrix(
                    ray,
                    *(terrainObj->modelPtr),
                    terrainObj->modelMatrix,
                    outT,
                    outPoint
                );
                if (hit) {
                    // Place orc at that intersection
                    SceneObject* orcObj = getSceneObjectById(2);
                    if (orcObj) {
                        glm::vec3 currentPos = glm::vec3(orcObj->modelMatrix[3]);
                        orcObj->isMoving      = true;
                        orcObj->moveStartTime = static_cast<float>(glfwGetTime());
                        orcObj->moveDuration  = 2.0f;
                        orcObj->moveStartPos  = currentPos;
                        orcObj->moveEndPos    = outPoint;

                        updateWorldBounds(*orcObj);
                        std::cout << "Orc placed at: "
                                  << outPoint.x << ", "
                                  << outPoint.y << ", "
                                  << outPoint.z << "\n";
                    }
                }
                else {
                    std::cout << "No mesh intersection with terrain!\n";
                }
            }
            orcSelected = false;
        }
    }
}


void renderSkybox() {
    // 2) Draw the skybox last, using a view matrix WITHOUT the camera translation.
//    This prevents the skybox from “sliding” or “spinning around” weirdly when you move.
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

    // Now actually draw the cube for your skybox
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
        processMovement();

        // 1) render depth from the light's POV
        glDisable(GL_CULL_FACE);
        renderSceneFromLight();
        glEnable(GL_CULL_FACE);


        // 2) render the scene from the camera's POV
        renderScene();

      //renderSkybox();

      

        // Animate moving objects
        {
            double currentTime = glfwGetTime();
            glm::mat4 dragonModelMatrix; // To store dragon's current modelMatrix

            for (auto& obj : g_sceneObjects) {
     
                float animationRadius = 60.0f;       // Radius of the circular path
                float animationSpeed = 0.7f;
                if (obj.id == 5) {  // Check if this is the dragon
                    // Compute the new position using circular motion
                    float angle = animationSpeed * (float)currentTime;  // Angle changes with time
                    float x = animationRadius * cos(angle);             // X position
                    float z = animationRadius * sin(angle);             // Z position
                    float y = 60.0f + 20.0f * sin(0.5f * angle);       // Y oscillates up and down

                    float rotationAngle = 1 * (float)currentTime;  // Rotation in degrees
                   // glm::mat4 R = glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));  // Rotate around Y-axis

                    // Update the dragon's model matrix for the new position
                    glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
                    glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));  // Scale remains constant
                    // Rotate the object as it moves along the circle
                    //obj.modelMatrix = glm::rotate(obj.modelMatrix, angle, glm::vec3(0.0f, 1.0f, 0.0f));  // Rotate around Y-axis

                    obj.modelMatrix = T * S;  // Apply translation and scaling
                    obj.modelMatrix = glm::rotate(obj.modelMatrix, angle, glm::vec3(0.0f, 1.0f, 0.0f));  // Rotate around Y-axis
                    // Update the dragon's bounding sphere to match the new position
                    updateWorldBounds(obj);

                    dragonModelMatrix = obj.modelMatrix;
                }
                else
                    if (obj.id == 10 || obj.id == 11) { // Left and Right Wings
                        // Define flapping parameters
                        float flapSpeed = 1.0f;       // Flaps per second
                        float flapAmplitude = 2.0f;  // Max rotation angle in degrees

                        // Calculate flapping angle based on time
                        float wingFlapAngle = glm::radians(flapAmplitude) * sin(currentTime * flapSpeed * 2.0f * glm::pi<float>());

                        // Determine wing offset (consistent with initial offset)
                        glm::vec3 wingOffset;
                        if (obj.id == 10) { // Left Wing
                            wingOffset = glm::vec3(-55.0f, 5.0f, -55.0f); // Same as initial offset
                        }
                        else { // Right Wing
                            wingOffset = glm::vec3(-55.0f, 5.0f, -55.0f);  // Same as initial offset
                            wingFlapAngle = -wingFlapAngle; // Opposite rotation for symmetry
                        }

                        // Create the rotation matrix for flapping
                        glm::mat4 R = glm::rotate(glm::mat4(1.0f), wingFlapAngle, glm::vec3(1.0f, 0.0f, 0.0f)); // Flap around X-axis

                        // Update the wing's modelMatrix based on the dragon's modelMatrix
                        obj.modelMatrix =  glm::translate(glm::mat4(1.0f), wingOffset) * R * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
                        //obj.modelMatrix = R;
                        // Update bounding sphere
                        updateWorldBounds(obj);
                    } 
                else 
                if (obj.isMoving) {
                    float elapsed = static_cast<float>(currentTime) - obj.moveStartTime;
                    float alpha   = elapsed / obj.moveDuration;

                    if (alpha >= 1.0f) {
                        alpha = 1.0f;
                        obj.isMoving = false;
                    }
                   // glm::vec3 newPos = glm::mix(obj.moveStartPos, obj.moveEndPos, alpha);
                   // obj.modelMatrix   = glm::translate(glm::mat4(1.0f), newPos);
                    glm::vec3 newPos = glm::mix(obj.moveStartPos, obj.moveEndPos, alpha);
                    glm::mat4 T = glm::translate(glm::mat4(1.0f), newPos);
                    glm::mat4 S = glm::scale(glm::mat4(1.0f), obj.scale);

                    obj.modelMatrix = T * S;

                    updateWorldBounds(obj);
                }
            }
        }

        glfwPollEvents();
        glfwSwapBuffers(glWindow);
    }

    cleanup();
    return 0;
}
