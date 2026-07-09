//
// GuiManager.hpp
// GUI Manager pentru RTS � layout stil Warcraft/Starcraft
//

#ifndef GUI_MANAGER_HPP
#define GUI_MANAGER_HPP

#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include "imGUI/imgui.h"
#include "imGUI/imgui_impl_glfw.h"
#include "imGUI/imgui_impl_opengl3.h"

#include <string>
#include <functional>
#include <vector>
#include <glm/glm.hpp>

#include "SelfTest.hpp"

namespace gps {

    // Forward declarations
    class Scene;
    class SceneManager;
    class SelectionSystem;
    class TileManager;
    class EditorState;
    class BenchmarkHarness;

    struct GuiButton {
        std::string label;
        std::function<void()> callback;
        glm::vec4 color = glm::vec4(0.0f);
        std::string tooltip = "";
        bool enabled = true;
    };

    class GuiManager {
    public:
        GuiManager();
        ~GuiManager();

        GuiManager(const GuiManager&) = delete;
        GuiManager& operator=(const GuiManager&) = delete;

        // Lifecycle
        void Initialize(GLFWwindow* window, const char* glslVersion = "#version 410");
        void Shutdown();
        void BeginFrame();
        void EndFrame();

        // Render all panels
        void RenderAllPanels();

        // Individual panels
        void RenderTopBar();
        void RenderCommandPanel();
        void RenderUnitInfoPanel();
        void RenderDebugPanel();
        // Sectiune in panoul de depanare: teste functionale + benchmark de performanta.
        void RenderBenchmarkSection();
        void RenderSpawnPanel();
        void RenderInspectorPanel();
        void RenderHelpOverlay();
        void RenderMinimap();
        void RenderPauseOverlay();
        void RenderGameStateOverlay();
        void RenderStartOverlay();
        // Billboard HP bars projected above every alive unit.
        void RenderHealthBars();

        // Editor state
        void BindEditorState(EditorState* editorState) { m_editorState = editorState; }

        // Benchmark harness (capitolul 6) - condus din sectiunea Debug panel.
        void BindBenchmark(BenchmarkHarness* benchmark) { m_benchmark = benchmark; }

        // Button system
        void AddButton(const GuiButton& button);
        void ClearButtons();

        // Data binding
        void BindSystems(Scene* scene, SceneManager* sceneManager, SelectionSystem* selectionSystem, TileManager* tileManager = nullptr);
        void SetCameraPosition(const glm::vec3& pos) { m_cameraPos = pos; }
        void SetDeltaTime(float dt) { m_deltaTime = dt; }
        void SetZoomFactor(float zoom) { m_zoomFactor = zoom; }
        // Cached every frame from main so RenderHealthBars can project world -> screen.
        void SetViewProjection(const glm::mat4& view, const glm::mat4& proj) { m_view = view; m_proj = proj; }

        // Panel toggles
        void SetDebugPanelVisible(bool v) { m_showDebugPanel = v; }
        bool IsDebugPanelVisible() const { return m_showDebugPanel; }

        void ToggleHelpOverlay() { m_showHelp = !m_showHelp; }
        bool IsHelpOverlayVisible() const { return m_showHelp; }

        void TogglePaused() { m_paused = !m_paused; }
        bool IsPaused() const { return m_paused; }

        void SetVictory(bool v) { m_victory = v; }
        void SetDefeat(bool v)  { m_defeat = v; }

        // Seconds of pre-combat grace remaining at match start. <= 0 means
        // grace is over (overlay hidden). Driven each frame from main.
        void SetGraceSecRemaining(float s) { m_graceSecRemaining = s; }

        // Match flow: pre-game start screen + post-game stats + reset handshake.
        bool IsGameStarted() const { return m_gameStarted; }
        void SetGameStarted(bool v) { m_gameStarted = v; }
        bool ConsumeResetRequest() { bool v = m_resetRequested; m_resetRequested = false; return v; }
        void SetMatchStats(float elapsedSec, int p1Alive, int p2Alive, int p1Lost, int p2Lost, float oilSpent) {
            m_matchElapsed = elapsedSec;
            m_matchP1Alive = p1Alive; m_matchP2Alive = p2Alive;
            m_matchP1Lost  = p1Lost;  m_matchP2Lost  = p2Lost;
            m_matchOilSpent = oilSpent;
        }

        // Returns true and writes a world point (Y=0) when the user clicked the minimap
        // this frame, so the camera can be teleported. False otherwise.
        bool ConsumeMinimapClick(glm::vec3& outWorldPoint);

        // Mouse/keyboard capture
        bool WantsMouseInput() const;
        bool WantsKeyboardInput() const;
        // True only while an ImGui text field is actively focused. Use this
        // (not WantsKeyboardInput) to gate game hotkeys, otherwise the nav
        // keyboard flag keeps WantCaptureKeyboard sticky after any panel click.
        bool WantsTextInput() const;

        // Debug toggles � citeste-le din main.cpp
        bool isWireframeEnabled = false;
        bool showCollisionBoxes = false;
        bool showBoundingSpheres = false;

        // Spawn config � editabile din GUI
        int spawnFormationCount = 10;
        float spawnFormationSpacing = 15.0f;

    private:
        bool m_initialized;
        GLFWwindow* m_window;

        bool m_showDebugPanel;

        std::vector<GuiButton> m_buttons;

        // System references
        Scene* m_scene;
        SceneManager* m_sceneManager;
        SelectionSystem* m_selectionSystem;
        TileManager* m_tileManager;
        EditorState* m_editorState;
        BenchmarkHarness* m_benchmark = nullptr;

        // Ultimul raport al testelor functionale, afisat sub buton.
        TestReport m_lastReport;
        bool       m_hasReport = false;

        // Tile spawn config
        int m_tileGridSize;

        // Cached data
        glm::vec3 m_cameraPos;
        float m_deltaTime;
        float m_zoomFactor;
        glm::mat4 m_view = glm::mat4(1.0f);
        glm::mat4 m_proj = glm::mat4(1.0f);

        // FPS tracking (media pe mai multe frame-uri)
        float m_fpsHistory[120];
        int m_fpsHistoryIdx;

        void ApplyCustomStyle();

        // Helper: deseneaza un buton colorat cu dimensiune fixa
        bool ColoredButton(const char* label, ImVec2 size, ImVec4 color, ImVec4 hoverColor);

        int m_tileSize;
        int m_tileModelScale;

        int heightTile;

        // New overlays / game state
        bool m_showHelp;
        bool m_paused;
        bool m_victory;
        bool m_defeat;

        // Match flow
        bool  m_gameStarted    = false;
        bool  m_resetRequested = false;
        float m_matchElapsed   = 0.0f;
        int   m_matchP1Alive   = 0;
        int   m_matchP2Alive   = 0;
        int   m_matchP1Lost    = 0;
        int   m_matchP2Lost    = 0;
        float m_matchOilSpent  = 0.0f;

        // Minimap click teleport — set in RenderMinimap, drained by main loop
        bool      m_minimapClickPending;
        glm::vec3 m_minimapClickWorldPoint;

        // Pre-combat grace window countdown ("Battle starts in 3..."). Driven
        // from main each frame; <=0 hides the overlay.
        float     m_graceSecRemaining = 0.0f;

    public:
        // Floating damage numbers — one entry per damage hit, drained over
        // ~1s by RenderHealthBars. Pushed from main.cpp's ApplyDamageTo.
        struct FloatingNumber {
            glm::vec3 worldPos;
            float     amount;
            float     age;
            bool      kill; // red if true (lethal), orange otherwise
        };
        void PushFloatingNumber(const glm::vec3& worldPos, int amount, bool kill);

    private:
        std::vector<FloatingNumber> m_floatingNumbers;
    };

} 

#endif