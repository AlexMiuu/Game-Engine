//
// SelectionSystem.hpp
// Sistem pentru selec?ia unit�?ilor (box selection + single click)
//

#ifndef SELECTION_SYSTEM_HPP
#define SELECTION_SYSTEM_HPP

#include "Scene.hpp"
#include "SceneObject.hpp"
#include "Camera.hpp"
#include "Shader.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_set>
#include <memory>
#include "RayCaster.hpp"

#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

namespace gps {

    /**
     * @brief Sistem pentru selec?ia unit�?ilor
     *
     * Features:
     * - Box selection (drag mouse pentru dreptunghi)
     * - Single selection (click pe unitate)
     * - Additive selection (SHIFT + click/drag)
     * - Visual feedback (box rendering)
     * - Filtrare pe tip de obiect (selecteaz� doar trupe)
     *
     * Exemplu de utilizare:
     * @code
     * SelectionSystem selection;
     * selection.Initialize(1920, 1080);
     * selection.LoadShader("shaders/selection.vert", "shaders/selection.frag");
     *
     * // �n mouse button callback:
     * if (mousePressed) {
     *     selection.StartBoxSelection(mousePos);
     * }
     * if (mouseReleased) {
     *     selection.EndBoxSelection(scene, camera, projection, shiftHeld);
     * }
     *
     * // �n render loop:
     * selection.Render();
     *
     * // Query selected:
     * for (int id : selection.GetSelectedIDs()) {
     *     // ...
     * }
     * @endcode
     */
    class SelectionSystem {
    public:
        SelectionSystem();
        ~SelectionSystem();

        // Prevent copying
        SelectionSystem(const SelectionSystem&) = delete;
        SelectionSystem& operator=(const SelectionSystem&) = delete;

        // ===========================
        // INITIALIZATION
        // ===========================

        /**
         * @brief Ini?ializeaz� sistemul de selec?ie
         * @param screenWidth L�?imea ecranului
         * @param screenHeight �n�l?imea ecranului
         */
        void Initialize(int screenWidth, int screenHeight);

        /**
         * @brief �ncarc� shader-ul pentru rendering box
         * @param vertPath Path la vertex shader
         * @param fragPath Path la fragment shader
         */
        void LoadShader(const std::string& vertPath, const std::string& fragPath);

        /**
         * @brief Cleanup resurse OpenGL
         */
        void Cleanup();

        // ===========================
        // UPDATE
        // ===========================

        /**
         * @brief Update selection system
         */
        void Update(float deltaTime);

        // ===========================
        // BOX SELECTION
        // ===========================

        /**
         * @brief �ncepe box selection
         * @param screenPos Pozi?ia mouse-ului pe ecran (pixeli)
         */
        void StartBoxSelection(const glm::vec2& screenPos);

        /**
         * @brief Actualizeaz� box selection (mouse drag)
         * @param screenPos Pozi?ia curent� a mouse-ului
         */
        void UpdateBoxSelection(const glm::vec2& screenPos);

        /**
         * @brief Finalizeaz� box selection ?i selecteaz� obiectele
         * @param scene Scena cu obiecte
         * @param camera Camera pentru proiec?ii
         * @param projection Matricea de proiec?ie
         * @param additive Dac� true, adaug� la selec?ia existent� (SHIFT held)
         */
        void EndBoxSelection(Scene& scene, const Camera& camera,
            const glm::mat4& projection, bool additive);

        /**
         * @brief Verific� dac� box selection e activ�
         */
        bool IsBoxSelecting() const { return m_isBoxSelecting; }

        // ===========================
        // SINGLE SELECTION
        // ===========================

        /**
         * @brief Selecteaz� un singur obiect la pozi?ia mouse-ului
         * @param scene Scena
         * @param camera Camera
         * @param projection Matricea de proiec?ie
         * @param screenPos Pozi?ia mouse-ului
         * @param additive Dac� true, adaug� la selec?ie
         */
        void SelectAtPoint(Scene& scene, const Camera& camera,
            const glm::mat4& projection,
            const glm::vec2& screenPos, bool additive);

        // ===========================
        // SELECTION QUERIES
        // ===========================

        /**
         * @brief Ob?ine set-ul de ID-uri selectate
         */
        const std::unordered_set<int>& GetSelectedIDs() const { return m_selectedIDs; }

        /**
         * @brief Verific� dac� un obiect e selectat
         */
        bool IsSelected(int id) const;

        /**
         * @brief Ob?ine num�rul de obiecte selectate
         */
        size_t GetSelectionCount() const { return m_selectedIDs.size(); }

        /**
         * @brief Verific� dac� exist� selec?ie
         */
        bool HasSelection() const { return !m_selectedIDs.empty(); }

        // ===========================
        // SELECTION MANIPULATION
        // ===========================

        /**
         * @brief Gole?te selec?ia
         */
        void ClearSelection();

        /**
         * @brief Adaug� un obiect la selec?ie
         */
        void AddToSelection(int id);

        /**
         * @brief ?terge un obiect din selec?ie
         */
        void RemoveFromSelection(int id);

        /**
         * @brief Selecteaz� toate obiectele de un anumit tip
         * @param scene Scena
         * @param nameFilter Filtreaz� dup� nume (ex: "Troop", "Orc")
         */
        void SelectAllOfType(Scene& scene, const std::string& nameFilter);

        // ===========================
        // RENDERING
        // ===========================

        /**
         * @brief Deseneaz� box-ul de selec?ie
         */
        void Render();

        /**
         * @brief Seteaz� culoarea box-ului
         */
        void SetBoxColor(const glm::vec4& color) { m_boxColor = color; }

        /**
         * @brief Seteaz� culoarea highlight pentru obiecte selectate
         */
        void SetHighlightColor(const glm::vec3& color) { m_highlightColor = color; }

        // ===========================
        // CONFIGURATION
        // ===========================

        /**
         * @brief Seteaz� dimensiunea ecranului (pentru conversii NDC)
         */
        void SetScreenSize(int width, int height);

        /**
         * @brief Activeaz�/dezactiveaz� highlight vizual
         */
        void SetHighlightEnabled(bool enabled) { m_highlightEnabled = enabled; }

        /**
         * @brief Seteaz� dimensiunea minim� a box-ului pentru a fi valid
         */
        void SetMinBoxSize(float size) { m_minBoxSize = size; }

        int GetObjectAtPoint(Scene& scene, const Camera& camera,const glm::mat4& projection,const glm::vec2& screenPos) const;

        void SetEditModeSelection(bool enabled) { m_editModeSelection = enabled; }
        
    private:
        // ===========================
        // STATE
        // ===========================
        std::unordered_set<int> m_selectedIDs;

        // Box selection state
        bool m_isBoxSelecting;
        glm::vec2 m_boxStart;
        glm::vec2 m_boxEnd;

        // Screen dimensions
        int m_screenWidth;
        int m_screenHeight;

        // Rendering
        GLuint m_boxVAO;
        GLuint m_boxVBO;
        Shader m_boxShader;
        bool m_shaderLoaded;

        // Visual settings
        glm::vec4 m_boxColor;
        glm::vec3 m_highlightColor;
        bool m_highlightEnabled;
        float m_minBoxSize;

        bool m_initialized;
        bool m_editModeSelection =false;
        // ===========================
        // INTERNAL HELPERS
        // ===========================

        /**
         * @brief Ini?ializeaz� resursele OpenGL pentru rendering
         */
        void InitializeRenderResources();

        /**
         * @brief Verific� dac� un obiect poate fi selectat
         */
        bool IsSelectable(const SceneObject& obj) const;

        /**
         * @brief Verific� dac� un obiect e �n box-ul de selec?ie
         */
        bool IsObjectInSelectionBox(const SceneObject& obj,
            const Camera& camera,
            const glm::mat4& projection);

        /**
         * @brief Converte?te coordonate screen �n NDC
         */
        glm::vec2 ScreenToNDC(const glm::vec2& screenPos) const;

        /**
         * @brief Proiecteaz� un punct 3D �n screen space
         */
        glm::vec3 ProjectToScreen(const glm::vec3& worldPos,
            const Camera& camera,
            const glm::mat4& projection) const;

        /**
         * @brief Verific� dac� un punct e �n dreptunghi NDC
         */
        bool IsPointInBox(const glm::vec2& point,
            const glm::vec2& boxMin,
            const glm::vec2& boxMax) const;

        /**
         * @brief Actualizeaz� mesh-ul box-ului
         */
        void UpdateBoxMesh();

        /**
         * @brief Setup OpenGL state pentru rendering UI
         */
        void SetupRenderState();

        /**
         * @brief Restore OpenGL state dup� rendering
         */
        void RestoreRenderState();


        Ray ComputeOrthoRay(const glm::vec2& screenPos,
            const Camera& camera,
            const glm::mat4& projection) const;
    };

} // namespace gps

#endif // SELECTION_SYSTEM_HPP