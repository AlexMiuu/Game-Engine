//
// SelectionSystem.hpp
// Sistem pentru selec?ia unitã?ilor (box selection + single click)
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
     * @brief Sistem pentru selec?ia unitã?ilor
     *
     * Features:
     * - Box selection (drag mouse pentru dreptunghi)
     * - Single selection (click pe unitate)
     * - Additive selection (SHIFT + click/drag)
     * - Visual feedback (box rendering)
     * - Filtrare pe tip de obiect (selecteazã doar trupe)
     *
     * Exemplu de utilizare:
     * @code
     * SelectionSystem selection;
     * selection.Initialize(1920, 1080);
     * selection.LoadShader("shaders/selection.vert", "shaders/selection.frag");
     *
     * // În mouse button callback:
     * if (mousePressed) {
     *     selection.StartBoxSelection(mousePos);
     * }
     * if (mouseReleased) {
     *     selection.EndBoxSelection(scene, camera, projection, shiftHeld);
     * }
     *
     * // În render loop:
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
         * @brief Ini?ializeazã sistemul de selec?ie
         * @param screenWidth Lã?imea ecranului
         * @param screenHeight Înãl?imea ecranului
         */
        void Initialize(int screenWidth, int screenHeight);

        /**
         * @brief Încarcã shader-ul pentru rendering box
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
         * @brief Începe box selection
         * @param screenPos Pozi?ia mouse-ului pe ecran (pixeli)
         */
        void StartBoxSelection(const glm::vec2& screenPos);

        /**
         * @brief Actualizeazã box selection (mouse drag)
         * @param screenPos Pozi?ia curentã a mouse-ului
         */
        void UpdateBoxSelection(const glm::vec2& screenPos);

        /**
         * @brief Finalizeazã box selection ?i selecteazã obiectele
         * @param scene Scena cu obiecte
         * @param camera Camera pentru proiec?ii
         * @param projection Matricea de proiec?ie
         * @param additive Dacã true, adaugã la selec?ia existentã (SHIFT held)
         */
        void EndBoxSelection(Scene& scene, const Camera& camera,
            const glm::mat4& projection, bool additive);

        /**
         * @brief Verificã dacã box selection e activã
         */
        bool IsBoxSelecting() const { return m_isBoxSelecting; }

        // ===========================
        // SINGLE SELECTION
        // ===========================

        /**
         * @brief Selecteazã un singur obiect la pozi?ia mouse-ului
         * @param scene Scena
         * @param camera Camera
         * @param projection Matricea de proiec?ie
         * @param screenPos Pozi?ia mouse-ului
         * @param additive Dacã true, adaugã la selec?ie
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
         * @brief Verificã dacã un obiect e selectat
         */
        bool IsSelected(int id) const;

        /**
         * @brief Ob?ine numãrul de obiecte selectate
         */
        size_t GetSelectionCount() const { return m_selectedIDs.size(); }

        /**
         * @brief Verificã dacã existã selec?ie
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
         * @brief Adaugã un obiect la selec?ie
         */
        void AddToSelection(int id);

        /**
         * @brief ?terge un obiect din selec?ie
         */
        void RemoveFromSelection(int id);

        /**
         * @brief Selecteazã toate obiectele de un anumit tip
         * @param scene Scena
         * @param nameFilter Filtreazã dupã nume (ex: "Troop", "Orc")
         */
        void SelectAllOfType(Scene& scene, const std::string& nameFilter);

        // ===========================
        // RENDERING
        // ===========================

        /**
         * @brief Deseneazã box-ul de selec?ie
         */
        void Render();

        /**
         * @brief Seteazã culoarea box-ului
         */
        void SetBoxColor(const glm::vec4& color) { m_boxColor = color; }

        /**
         * @brief Seteazã culoarea highlight pentru obiecte selectate
         */
        void SetHighlightColor(const glm::vec3& color) { m_highlightColor = color; }

        // ===========================
        // CONFIGURATION
        // ===========================

        /**
         * @brief Seteazã dimensiunea ecranului (pentru conversii NDC)
         */
        void SetScreenSize(int width, int height);

        /**
         * @brief Activeazã/dezactiveazã highlight vizual
         */
        void SetHighlightEnabled(bool enabled) { m_highlightEnabled = enabled; }

        /**
         * @brief Seteazã dimensiunea minimã a box-ului pentru a fi valid
         */
        void SetMinBoxSize(float size) { m_minBoxSize = size; }

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

        // ===========================
        // INTERNAL HELPERS
        // ===========================

        /**
         * @brief Ini?ializeazã resursele OpenGL pentru rendering
         */
        void InitializeRenderResources();

        /**
         * @brief Verificã dacã un obiect poate fi selectat
         */
        bool IsSelectable(const SceneObject& obj) const;

        /**
         * @brief Verificã dacã un obiect e în box-ul de selec?ie
         */
        bool IsObjectInSelectionBox(const SceneObject& obj,
            const Camera& camera,
            const glm::mat4& projection);

        /**
         * @brief Converte?te coordonate screen în NDC
         */
        glm::vec2 ScreenToNDC(const glm::vec2& screenPos) const;

        /**
         * @brief Proiecteazã un punct 3D în screen space
         */
        glm::vec3 ProjectToScreen(const glm::vec3& worldPos,
            const Camera& camera,
            const glm::mat4& projection) const;

        /**
         * @brief Verificã dacã un punct e în dreptunghi NDC
         */
        bool IsPointInBox(const glm::vec2& point,
            const glm::vec2& boxMin,
            const glm::vec2& boxMax) const;

        /**
         * @brief Actualizeazã mesh-ul box-ului
         */
        void UpdateBoxMesh();

        /**
         * @brief Setup OpenGL state pentru rendering UI
         */
        void SetupRenderState();

        /**
         * @brief Restore OpenGL state dupã rendering
         */
        void RestoreRenderState();



    };

} // namespace gps

#endif // SELECTION_SYSTEM_HPP