//
// SelectionSystem.cpp
// Implementarea sistemului de selec?ie
//

#include "SelectionSystem.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace gps {

    SelectionSystem::SelectionSystem()
        : m_isBoxSelecting(false)
        , m_screenWidth(1920)
        , m_screenHeight(1080)
        , m_boxVAO(0)
        , m_boxVBO(0)
        , m_shaderLoaded(false)
        , m_boxColor(0.3f, 0.6f, 1.0f, 0.3f)  // Albastru transparent
        , m_highlightColor(1.0f, 1.0f, 0.0f)  // Galben
        , m_highlightEnabled(true)
        , m_minBoxSize(5.0f)  // 5 pixeli minim
        , m_initialized(false)
    {
    }

    SelectionSystem::~SelectionSystem() {
        Cleanup();
    }

    // ===========================
    // INITIALIZATION
    // ===========================

    void SelectionSystem::Initialize(int screenWidth, int screenHeight) {
        m_screenWidth = screenWidth;
        m_screenHeight = screenHeight;

        InitializeRenderResources();

        m_initialized = true;
        std::cout << "? SelectionSystem initialized (" << screenWidth << "x" << screenHeight << ")" << std::endl;
    }

    void SelectionSystem::LoadShader(const std::string& vertPath, const std::string& fragPath) {
        m_boxShader.loadShader(vertPath, fragPath);
        m_shaderLoaded = true;
        std::cout << "? Selection shader loaded" << std::endl;
    }

    void SelectionSystem::Cleanup() {
        if (m_boxVAO != 0) {
            glDeleteVertexArrays(1, &m_boxVAO);
            m_boxVAO = 0;
        }
        if (m_boxVBO != 0) {
            glDeleteBuffers(1, &m_boxVBO);
            m_boxVBO = 0;
        }
        m_initialized = false;
    }

    void SelectionSystem::InitializeRenderResources() {
        // Creeazã VAO ?i VBO pentru box
        glGenVertexArrays(1, &m_boxVAO);
        glGenBuffers(1, &m_boxVBO);

        glBindVertexArray(m_boxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_boxVBO);

        // Alocã spa?iu pentru 4 vertices (quad)
        glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

        // Position attribute (2D - screen space)
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        std::cout << "? Selection render resources initialized" << std::endl;
    }

    // ===========================
    // UPDATE
    // ===========================

    void SelectionSystem::Update(float deltaTime) {
        // Poate fi folosit pentru anima?ii de selec?ie, fade effects, etc.
        // Pentru moment nu e necesar
    }

    // ===========================
    // BOX SELECTION
    // ===========================

    void SelectionSystem::StartBoxSelection(const glm::vec2& screenPos) {
        m_isBoxSelecting = true;
        m_boxStart = screenPos;
        m_boxEnd = screenPos;
    }

    void SelectionSystem::UpdateBoxSelection(const glm::vec2& screenPos) {
        if (m_isBoxSelecting) {
            m_boxEnd = screenPos;
        }
    }

    void SelectionSystem::EndBoxSelection(Scene& scene, const Camera& camera,
        const glm::mat4& projection, bool additive) {
        if (!m_isBoxSelecting) return;

        // Verificã dimensiunea minimã a box-ului
        float boxWidth = std::abs(m_boxEnd.x - m_boxStart.x);
        float boxHeight = std::abs(m_boxEnd.y - m_boxStart.y);

        if (boxWidth < m_minBoxSize && boxHeight < m_minBoxSize) {
            // Box prea mic - trateazã ca single click
            m_isBoxSelecting = false;
            SelectAtPoint(scene, camera, projection, m_boxStart, additive);
            return;
        }

        // Gole?te selec?ia dacã nu e additive
        if (!additive) {
            ClearSelection();
        }

        // Selecteazã obiecte în box
        int selectedCount = 0;
        for (const auto& objPtr : scene.GetObjects()) {
            SceneObject* obj = objPtr.get();

            if (!IsSelectable(*obj)) continue;

            if (IsObjectInSelectionBox(*obj, camera, projection)) {
                AddToSelection(obj->GetID());
                selectedCount++;
            }
        }

        m_isBoxSelecting = false;

        std::cout << "?? Box selection: " << selectedCount << " objects selected (total: "
            << m_selectedIDs.size() << ")" << std::endl;
    }

    // ===========================
    // SINGLE SELECTION
    // ===========================

    void SelectionSystem::SelectAtPoint(Scene& scene, const Camera& camera,
        const glm::mat4& projection,
        const glm::vec2& screenPos, bool additive) {
        // TODO: Implementeazã raycasting pentru single selection
        // Pentru simplitate, pentru moment doar gole?te selec?ia dacã nu e additive

        if (!additive) {
            ClearSelection();
        }

        // Ar trebui sã faci raycast ?i sã gãse?ti obiectul cel mai apropiat
        // Pentru moment, doar log
        std::cout << "?? Single selection at (" << screenPos.x << ", " << screenPos.y << ")" << std::endl;
    }

    // ===========================
    // SELECTION QUERIES
    // ===========================

    bool SelectionSystem::IsSelected(int id) const {
        return m_selectedIDs.find(id) != m_selectedIDs.end();
    }

    // ===========================
    // SELECTION MANIPULATION
    // ===========================

    void SelectionSystem::ClearSelection() {
        m_selectedIDs.clear();
    }

    void SelectionSystem::AddToSelection(int id) {
        m_selectedIDs.insert(id);
    }

    void SelectionSystem::RemoveFromSelection(int id) {
        m_selectedIDs.erase(id);
    }

    void SelectionSystem::SelectAllOfType(Scene& scene, const std::string& nameFilter) {
        ClearSelection();

        for (const auto& objPtr : scene.GetObjects()) {
            SceneObject* obj = objPtr.get();

            if (obj->GetName().find(nameFilter) != std::string::npos) {
                if (IsSelectable(*obj)) {
                    AddToSelection(obj->GetID());
                }
            }
        }

        std::cout << "?? Selected all of type '" << nameFilter << "': "
            << m_selectedIDs.size() << " objects" << std::endl;
    }

    // ===========================
    // RENDERING
    // ===========================

    void SelectionSystem::Render() {
        if (!m_isBoxSelecting) return;
        if (!m_initialized) return;

        // Verificã dimensiunea minimã pentru rendering
        float boxWidth = std::abs(m_boxEnd.x - m_boxStart.x);
        float boxHeight = std::abs(m_boxEnd.y - m_boxStart.y);

        if (boxWidth < 1.0f && boxHeight < 1.0f) return;

        // Setup render state
        SetupRenderState();

        // Update mesh
        UpdateBoxMesh();

        // Use shader (dacã e încãrcat) sau deseneazã fãrã shader
        if (m_shaderLoaded) {
            m_boxShader.useShaderProgram();
            GLuint colorLoc = glGetUniformLocation(m_boxShader.shaderProgram, "boxColor");
            glUniform4fv(colorLoc, 1, &m_boxColor[0]);
        }

        // Deseneazã box-ul (fill)
        glBindVertexArray(m_boxVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // Deseneazã marginea (outline)
        glLineWidth(2.0f);
        if (m_shaderLoaded) {
            GLuint colorLoc = glGetUniformLocation(m_boxShader.shaderProgram, "boxColor");
            glUniform4f(colorLoc, m_boxColor.r, m_boxColor.g, m_boxColor.b, 1.0f);  // Opac
        }
        glDrawArrays(GL_LINE_LOOP, 0, 4);

        glBindVertexArray(0);

        // Restore render state
        RestoreRenderState();
    }

    // ===========================
    // CONFIGURATION
    // ===========================

    void SelectionSystem::SetScreenSize(int width, int height) {
        m_screenWidth = width;
        m_screenHeight = height;
    }

    // ===========================
    // INTERNAL HELPERS
    // ===========================

    bool SelectionSystem::IsSelectable(const SceneObject& obj) const {
        // Selecteazã doar obiecte active cu anumite nume
        if (!obj.IsActive()) return false;

        std::string name = obj.GetName();

        // Selecteazã trupe, orci, etc. (dar NU terenul, zidurile)
        return (name.find("Troop") != std::string::npos ||
            name.find("Orc") != std::string::npos ||
            name.find("Unit") != std::string::npos);
    }

    bool SelectionSystem::IsObjectInSelectionBox(const SceneObject& obj,
        const Camera& camera,
        const glm::mat4& projection) {
        // Proiecteazã centrul obiectului în screen space
        glm::vec3 screenPos = ProjectToScreen(obj.GetWorldCenter(), camera, projection);

        // Verificã dacã e în spatele camerei
        if (screenPos.z < 0.0f || screenPos.z > 1.0f) {
            return false;
        }

        // Converte?te în NDC
        glm::vec2 startNDC = ScreenToNDC(m_boxStart);
        glm::vec2 endNDC = ScreenToNDC(m_boxEnd);

        // Normalizeazã (min, max)
        float minX = std::min(startNDC.x, endNDC.x);
        float maxX = std::max(startNDC.x, endNDC.x);
        float minY = std::min(startNDC.y, endNDC.y);
        float maxY = std::max(startNDC.y, endNDC.y);

        // Converte?te pozi?ia obiectului în NDC
        glm::vec2 objNDC = ScreenToNDC(glm::vec2(screenPos.x, screenPos.y));

        // Verificã dacã e în box
        return IsPointInBox(objNDC, glm::vec2(minX, minY), glm::vec2(maxX, maxY));
    }

    glm::vec2 SelectionSystem::ScreenToNDC(const glm::vec2& screenPos) const {
        float x = (2.0f * screenPos.x) / static_cast<float>(m_screenWidth) - 1.0f;
        float y = 1.0f - (2.0f * screenPos.y) / static_cast<float>(m_screenHeight);
        return glm::vec2(x, y);
    }

    glm::vec3 SelectionSystem::ProjectToScreen(const glm::vec3& worldPos,
        const Camera& camera,
        const glm::mat4& projection) const {
        glm::mat4 view = camera.getViewMatrix();

        // Transform to clip space
        glm::vec4 clipPos = projection * view * glm::vec4(worldPos, 1.0f);

        // Perspective divide
        if (clipPos.w != 0.0f) {
            clipPos /= clipPos.w;
        }

        // Convert to screen coordinates
        float screenX = (clipPos.x + 1.0f) * 0.5f * static_cast<float>(m_screenWidth);
        float screenY = (1.0f - clipPos.y) * 0.5f * static_cast<float>(m_screenHeight);

        return glm::vec3(screenX, screenY, clipPos.z);
    }

    bool SelectionSystem::IsPointInBox(const glm::vec2& point,
        const glm::vec2& boxMin,
        const glm::vec2& boxMax) const {
        return (point.x >= boxMin.x && point.x <= boxMax.x &&
            point.y >= boxMin.y && point.y <= boxMax.y);
    }

    void SelectionSystem::UpdateBoxMesh() {
        // Converte?te coordonatele screen în NDC pentru rendering
        glm::vec2 startNDC = ScreenToNDC(m_boxStart);
        glm::vec2 endNDC = ScreenToNDC(m_boxEnd);

        // 4 vertices pentru quad (în sens anti-orar)
        float vertices[] = {
            startNDC.x, startNDC.y,  // Bottom-left
            endNDC.x,   startNDC.y,  // Bottom-right
            endNDC.x,   endNDC.y,    // Top-right
            startNDC.x, endNDC.y     // Top-left
        };

        // Update VBO
        glBindBuffer(GL_ARRAY_BUFFER, m_boxVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void SelectionSystem::SetupRenderState() {
        // Salveazã starea curentã (op?ional - pentru acum, doar setãm ce avem nevoie)

        // Disable depth test pentru UI
        glDisable(GL_DEPTH_TEST);

        // Enable blending pentru transparen?ã
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Disable culling
        glDisable(GL_CULL_FACE);
    }

    void SelectionSystem::RestoreRenderState() {
        // Restore state
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
    }

} // namespace gps