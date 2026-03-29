//
// GuiManager.cpp
// GUI RTS-style: Top Bar, Command Panel, Unit Info, Debug
//

#include "GuiManager.hpp"
#include "Scene.hpp"
#include "SceneManager.hpp"
#include "SelectionSystem.hpp"
#include "TileManager.hpp"
#include "ResourceManager.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>

namespace gps {

    GuiManager::GuiManager()
        : m_initialized(false)
        , m_window(nullptr)
        , m_showDebugPanel(true)
        , m_scene(nullptr)
        , m_sceneManager(nullptr)
        , m_selectionSystem(nullptr)
        , m_tileManager(nullptr)
        , m_tileGridSize(5)
        , m_cameraPos(0.0f)
        , m_deltaTime(0.016f)
        , m_zoomFactor(1.0f)
        , m_fpsHistoryIdx(0)
        , m_tileSize(256)
        , m_tileModelScale(27)
        , heightTile(-60.0f)
    {
        memset(m_fpsHistory, 0, sizeof(m_fpsHistory));
    }

    GuiManager::~GuiManager() {
        if (m_initialized) Shutdown();
    }

    // ===========================
    // LIFECYCLE
    // ===========================

    void GuiManager::Initialize(GLFWwindow* window, const char* glslVersion) {
        m_window = window;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glslVersion);

        ApplyCustomStyle();

        m_initialized = true;
        std::cout << "GUI initialized (Dear ImGui " << IMGUI_VERSION << ")" << std::endl;
    }

    void GuiManager::Shutdown() {
        if (!m_initialized) return;
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        m_initialized = false;
    }

    void GuiManager::BeginFrame() {
        if (!m_initialized) return;
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void GuiManager::EndFrame() {
        if (!m_initialized) return;
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    // ===========================
    // RENDER ALL
    // ===========================

    void GuiManager::RenderAllPanels() {
        if (!m_initialized) return;

        // Update FPS history
        float fps = (m_deltaTime > 0.0f) ? (1.0f / m_deltaTime) : 0.0f;
        m_fpsHistory[m_fpsHistoryIdx] = fps;
        m_fpsHistoryIdx = (m_fpsHistoryIdx + 1) % 120;

        RenderTopBar();
        RenderSpawnPanel();
        RenderCommandPanel();
        RenderUnitInfoPanel();

        if (m_showDebugPanel) RenderDebugPanel();
    }

    // ===========================
    // TOP BAR — banda orizontala sus
    // ===========================

    void GuiManager::RenderTopBar() {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float barHeight = 32.0f;

        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y));
        ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, barHeight));

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 6));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.08f, 0.92f));

        ImGui::Begin("##TopBar", nullptr, flags);

        // FPS
        float fps = (m_deltaTime > 0.0f) ? (1.0f / m_deltaTime) : 0.0f;
        ImVec4 fpsColor = (fps >= 55.0f) ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f) :
            (fps >= 30.0f) ? ImVec4(1.0f, 1.0f, 0.3f, 1.0f) :
            ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
        ImGui::TextColored(fpsColor, "FPS: %.0f", fps);

        ImGui::SameLine(0, 30);
        ImGui::Separator();

        // Troops
        ImGui::SameLine(0, 30);
        if (m_sceneManager) {
            ImGui::Text("Troops: %d", m_sceneManager->GetTroopCount());
        }

        // Objects
        ImGui::SameLine(0, 30);
        if (m_scene) {
            ImGui::Text("Objects: %zu", m_scene->GetObjectCount());
        }

        // Tiles
        ImGui::SameLine(0, 30);
        if (m_tileManager) {
            ImGui::Text("Tiles: %d", m_tileManager->GetLoadedCount());
        }

        // Resources
        ImGui::SameLine(0, 16);
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f),
            "Oil: %.0f", gps::ResourceManager::Instance().Get("Oil"));

        // Selected
        ImGui::SameLine(0, 30);
        if (m_selectionSystem && m_selectionSystem->HasSelection()) {
            ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f),
                "Selected: %zu", m_selectionSystem->GetSelectionCount());
        }

        // Camera pos — dreapta
        ImGui::SameLine(viewport->WorkSize.x - 280);
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.7f, 1.0f),
            "Camera: (%.0f, %.0f, %.0f) Zoom: %.1fx",
            m_cameraPos.x, m_cameraPos.y, m_cameraPos.z, m_zoomFactor);

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }

    void GuiManager::RenderSpawnPanel(){
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        float panelWidth = 280.0f;
        float panelHeight = 620.0f;
        float margin = 10.0f;
        
        ImGui::SetNextWindowPos(ImVec2(10, 45), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(280, 400), ImGuiCond_FirstUseEver);


        ImGui::Begin("Spawn Units", nullptr);
        
        ImGui::SeparatorText("Combat Units");

            if (ColoredButton("Spawn Frigate", ImVec2(-1, 40),
                ImVec4(0.2f, 0.45f, 0.2f, 1.0f), ImVec4(0.25f, 0.6f, 0.25f, 1.0f)))
            {
                if (m_sceneManager) {
                    m_sceneManager->SetPropPlacement("ship","ship", glm::vec3(4.5f), "Ship");
                }
            }

        ImGui::SeparatorText("Resource Units");

                 if (ColoredButton("Oil Rig", ImVec2(-1, 40),
                ImVec4(0.2f, 0.45f, 0.2f, 1.0f), ImVec4(0.25f, 0.6f, 0.25f, 1.0f)))
            {
                if (m_sceneManager) {
                    m_sceneManager->SetPropPlacement("oilRig","oilRig", glm::vec3(2.5f), "OilRig");
                }
            }



        ImGui::End();
    }
    // ===========================
    // COMMAND PANEL — jos-dreapta, butoane de actiune
    // ===========================

    void GuiManager::RenderCommandPanel() {
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        float panelWidth = 280.0f;
        float panelHeight = 620.0f;
        float margin = 10.0f;

        ImGui::SetNextWindowPos(
            ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - panelWidth - margin,
                viewport->WorkPos.y + viewport->WorkSize.y - panelHeight - margin),
            ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight), ImGuiCond_FirstUseEver);

        ImGui::Begin("Commands", nullptr, ImGuiWindowFlags_NoCollapse);

        // ─── SPAWN ───
        ImGui::SeparatorText("Spawn Units");

        // Grid 2x2 de butoane spawn
        float btnWidth = (ImGui::GetContentRegionAvail().x - 8) * 0.5f;

        if (ColoredButton("Spawn Orc", ImVec2(btnWidth, 40),
            ImVec4(0.2f, 0.45f, 0.2f, 1.0f), ImVec4(0.25f, 0.6f, 0.25f, 1.0f)))
        {
            if (m_sceneManager) {
                glm::vec3 pos = m_sceneManager->GetTroopSpawnPosition();
                m_sceneManager->SpawnTroop(pos);
            }
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Hotkey: B");

        ImGui::SameLine();

        if (ColoredButton("Spawn Dragon", ImVec2(btnWidth, 40),
            ImVec4(0.5f, 0.18f, 0.18f, 1.0f), ImVec4(0.65f, 0.22f, 0.22f, 1.0f)))
        {
            if (m_sceneManager) {
                m_sceneManager->SpawnObject("Dragon", "dragon","dragon",
                    glm::vec3(50.0f, -60.0f, -50.0f), glm::vec3(0.5f));
            }
        }

        // Formation spawn cu slider
        ImGui::Spacing();
        ImGui::SliderInt("Count", &spawnFormationCount, 2, 30);
        ImGui::SliderFloat("Spacing", &spawnFormationSpacing, 5.0f, 40.0f, "%.0f");

        if (ColoredButton("Spawn Formation", ImVec2(-1, 35),
            ImVec4(0.2f, 0.3f, 0.5f, 1.0f), ImVec4(0.25f, 0.4f, 0.65f, 1.0f)))
        {
            if (m_sceneManager) {
                glm::vec3 pos = glm::vec3(100.0f, -60.0f, -90.0f);
                m_sceneManager->SpawnTroopFormation(pos, spawnFormationCount, spawnFormationSpacing);
            }
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Hotkey: N");

        // ─── SELECTION ───
        ImGui::SeparatorText("Selection");

        if (ImGui::Button("Select All Troops", ImVec2(btnWidth, 30))) {
            if (m_selectionSystem && m_scene) {
                m_selectionSystem->SelectAllOfType(*m_scene, "Troop");
                m_selectionSystem->SelectAllOfType(*m_scene, "Orc");
            }
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Hotkey: T");

        ImGui::SameLine();

        if (ImGui::Button("Clear Selection", ImVec2(btnWidth, 30))) {
            if (m_selectionSystem) {
                m_selectionSystem->ClearSelection();
            }
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Hotkey: C");

        // ─── TILE GRID ───
        if (m_tileManager) {
            ImGui::SeparatorText("Tile Grid");

            ImGui::Text("Loaded: %d / %d", m_tileManager->GetLoadedCount(), m_tileManager->GetTotalCount());
            ImGui::Text("Tile Size: %.0f", m_tileManager->GetTileSize());

            ImGui::SliderInt("Grid Size", &m_tileGridSize, 1, 7);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("NxN grid centered at origin");

            if (ColoredButton("Generate Grid", ImVec2(btnWidth, 35),
                ImVec4(0.3f, 0.35f, 0.2f, 1.0f), ImVec4(0.4f, 0.5f, 0.25f, 1.0f)))
            {
                int half = m_tileGridSize / 2;
                m_tileManager->Clear();
                m_tileManager->GenerateAndLoadGrid(-half, half, -half, half);
            }

            ImGui::SameLine();

            if (ColoredButton("Clear Tiles", ImVec2(btnWidth, 35),
                ImVec4(0.45f, 0.2f, 0.2f, 1.0f), ImVec4(0.6f, 0.25f, 0.25f, 1.0f)))
            {
                m_tileManager->Clear();
            }

            // Single tile add/remove
            ImGui::Spacing();
            ImGui::Text("Add/Remove Single Tile:");

            static int tileX = 0;
            static int tileZ = 0;
            ImGui::PushItemWidth(btnWidth - 20);
            ImGui::InputInt("Tile X", &tileX);
            ImGui::InputInt("Tile Z", &tileZ);
            ImGui::PopItemWidth();

            if (ImGui::Button("Add Tile", ImVec2(btnWidth, 28))) {
                m_tileManager->LoadTile(tileX, tileZ);
            }
            ImGui::SameLine();
            if (ImGui::Button("Remove Tile", ImVec2(btnWidth, 28))) {
                m_tileManager->UnloadTile(tileX, tileZ);
            }

            ImGui::Spacing();
            ImGui::SeparatorText("Resize");
            ImGui::InputInt("Grid Tile Size", &m_tileSize);
            ImGui::InputInt("Tile Model Scale", &m_tileModelScale);
            ImGui::InputInt("TILEHEIGHT", &heightTile);

            if (ImGui::Button("Apply Resize", ImVec2(btnWidth, 28)))
            {
                bool hadTiles = (m_tileManager->GetTotalCount() > 0);
                int minX = 0, maxX = 0, minZ = 0, maxZ = 0;

                if (hadTiles) {
                    m_tileManager->GetGridRange(minX, maxX, minZ, maxZ);
                }

                // Update size and scale. Grid spacing changes require rebuilding loaded tiles.
                m_tileManager->SetTileSize(static_cast<float>(m_tileSize));
                m_tileManager->SetTileModelScale(glm::vec3(m_tileModelScale));

                if (hadTiles) {
                    m_tileManager->Clear();
                    m_tileManager->GenerateAndLoadGrid(minX, maxX, minZ, maxZ);
                }

                m_tileManager->SetTileHeight(heightTile);

            }

        }

        // ─── CUSTOM BUTTONS ───
        if (!m_buttons.empty()) {
            ImGui::SeparatorText("Custom");
            for (auto& btn : m_buttons) {
                bool hasColor = (btn.color.r + btn.color.g + btn.color.b) > 0.0f;
                if (hasColor) {
                    ImGui::PushStyleColor(ImGuiCol_Button,
                        ImVec4(btn.color.r, btn.color.g, btn.color.b, btn.color.a));
                }
                if (!btn.enabled) ImGui::BeginDisabled();

                if (ImGui::Button(btn.label.c_str(), ImVec2(-1, 28))) {
                    if (btn.callback) btn.callback();
                }

                if (!btn.enabled) ImGui::EndDisabled();
                if (!btn.tooltip.empty() && ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", btn.tooltip.c_str());
                if (hasColor) ImGui::PopStyleColor();
            }
        }

        ImGui::End();
    }

    // ===========================
    // UNIT INFO — jos-centru, detalii selectie
    // ===========================

    void GuiManager::RenderUnitInfoPanel() {
        if (!m_selectionSystem || !m_selectionSystem->HasSelection()) return;
        if (!m_scene) return;

        ImGuiViewport* viewport = ImGui::GetMainViewport();

        float panelWidth = 320.0f;
        float panelHeight = 150.0f;
        float margin = 10.0f;

        // Centrat jos
        ImGui::SetNextWindowPos(
            ImVec2(viewport->WorkPos.x + (viewport->WorkSize.x - panelWidth) * 0.5f,
                viewport->WorkPos.y + viewport->WorkSize.y - panelHeight - margin),
            ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.06f, 0.10f, 0.92f));
        ImGui::Begin("Unit Info", nullptr, flags);

        const auto& selectedIDs = m_selectionSystem->GetSelectedIDs();
        size_t count = selectedIDs.size();

        if (count == 1) {
            // ─── SINGLE UNIT SELECTED ───
            int id = *selectedIDs.begin();
            SceneObject* obj = m_scene->GetObjectByID(id);

            if (obj) {
                // Nume mare
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.3f, 1.0f));
                ImGui::Text("%s", obj->GetName().c_str());
                ImGui::Text("%d", obj->unitStats.attack);
                ImGui::Text("%s", obj->GetTag());
                ImGui::PopStyleColor();

                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.0f), "(ID: %d)", obj->GetID());

                ImGui::Separator();

                // Position
                glm::vec3 pos = obj->GetTransform().GetPosition();
                ImGui::Text("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);

                // Scale
                glm::vec3 scl = obj->GetTransform().GetScale();
                ImGui::Text("Scale: (%.2f, %.2f, %.2f)", scl.x, scl.y, scl.z);

                // Bounding info
                ImGui::Text("Bounds radius: %.1f", obj->GetWorldRadius());

                // HP bar
                if (obj->unitStats.maxHealth > 0) {
                    float frac = (float)obj->unitStats.health / (float)obj->unitStats.maxHealth;
                    ImVec4 barColor = (frac > 0.6f) ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) :
                                     (frac > 0.3f) ? ImVec4(1.0f, 0.7f, 0.1f, 1.0f) :
                                                     ImVec4(0.9f, 0.15f, 0.15f, 1.0f);
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
                    char hpLabel[32];
                    snprintf(hpLabel, sizeof(hpLabel), "%d / %d HP",
                        obj->unitStats.health, obj->unitStats.maxHealth);
                    ImGui::ProgressBar(frac, ImVec2(-1.0f, 14.0f), hpLabel);
                    ImGui::PopStyleColor();

                    if (obj->unitStats.productionRate > 0.0f) {
                        ImGui::TextColored(ImVec4(0.8f, 0.9f, 0.4f, 1.0f),
                            "Producing: %.1f %s/s",
                            obj->unitStats.productionRate,
                            obj->unitStats.resourceType.c_str());
                    }
                }

                // Moving status
                if (obj->movement.isMoving) {
                    ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Moving...");

                    // Progress bar
                    float now = static_cast<float>(glfwGetTime());
                    float elapsed = now - obj->movement.moveStartTime;
                    float progress = std::min(elapsed / obj->movement.moveDuration, 1.0f);
                    ImGui::ProgressBar(progress, ImVec2(-1, 14));
                }
                else {
                    ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "Idle");
                }
            }
        }
        else {
            // ─── MULTIPLE UNITS SELECTED ───
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.3f, 1.0f));
            ImGui::Text("%zu units selected", count);
            ImGui::PopStyleColor();

            ImGui::Separator();

            // Breakdown pe tipuri
            int orcCount = 0;
            int troopCount = 0;
            int otherCount = 0;
            int movingCount = 0;

            for (int id : selectedIDs) {
                SceneObject* obj = m_scene->GetObjectByID(id);
                if (!obj) continue;

                std::string name = obj->GetName();
                if (name.find("Orc") != std::string::npos) orcCount++;
                else if (name.find("Troop") != std::string::npos) troopCount++;
                else otherCount++;

                if (obj->movement.isMoving) movingCount++;
            }

            if (orcCount > 0)   ImGui::Text("  Orcs: %d", orcCount);
            if (troopCount > 0) ImGui::Text("  Troops: %d", troopCount);
            if (otherCount > 0) ImGui::Text("  Other: %d", otherCount);

            ImGui::Spacing();
            if (movingCount > 0) {
                ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f),
                    "%d moving", movingCount);
            }
            else {
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "All idle");
            }

            // Lista scurta de ID-uri
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.0f), "IDs:");
            ImGui::SameLine();
            std::string idList;
            int shown = 0;
            for (int id : selectedIDs) {
                if (!idList.empty()) idList += ", ";
                idList += std::to_string(id);
                shown++;
                if (shown >= 8) {
                    idList += "...";
                    break;
                }
            }
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.0f), "%s", idList.c_str());
        }

        ImGui::End();
        ImGui::PopStyleColor();
    }

    // ===========================
    // DEBUG PANEL — F1 toggle
    // ===========================

    void GuiManager::RenderDebugPanel() {
        ImGui::SetNextWindowPos(ImVec2(10, 45), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(280, 400), ImGuiCond_FirstUseEver);

        ImGui::Begin("Debug [F1]", &m_showDebugPanel);

        // ─── RENDERING ───
        ImGui::SeparatorText("Rendering");

        if (ImGui::Checkbox("Wireframe", &isWireframeEnabled)) {
            glPolygonMode(GL_FRONT_AND_BACK, isWireframeEnabled ? GL_LINE : GL_FILL);
        }

        ImGui::Checkbox("Show Collision Boxes", &showCollisionBoxes);
        ImGui::Checkbox("Show Bounding Spheres", &showBoundingSpheres);

        // ─── PERFORMANCE ───
        ImGui::SeparatorText("Performance");

        float fps = (m_deltaTime > 0.0f) ? (1.0f / m_deltaTime) : 0.0f;
        ImGui::Text("FPS: %.1f  |  Frame: %.2f ms", fps, m_deltaTime * 1000.0f);

        // FPS graph
        ImGui::PlotLines("##FPSGraph", m_fpsHistory, 120, m_fpsHistoryIdx,
            "FPS", 0.0f, 120.0f, ImVec2(-1, 50));

        // ─── SCENE GRAPH ───
        ImGui::SeparatorText("Scene Graph");

        if (m_scene) {
            ImGui::Text("Total objects: %zu", m_scene->GetObjectCount());
            ImGui::Spacing();

            // Scrollable list
            ImGui::BeginChild("SceneObjects", ImVec2(0, 180), true);

            for (const auto& objPtr : m_scene->GetObjects()) {
                SceneObject* obj = objPtr.get();
                if (!obj) continue;

                bool isSel = m_selectionSystem ? m_selectionSystem->IsSelected(obj->GetID()) : false;

                // Icon based on type
                const char* icon = "  ";
                std::string name = obj->GetName();
                if (name.find("Orc") != std::string::npos || name.find("Troop") != std::string::npos)
                    icon = "[U]";  // Unit
                else if (name.find("Terrain") != std::string::npos || name.find("Tile") != std::string::npos)
                    icon = "[T]";  // Terrain
                else if (name.find("Dragon") != std::string::npos)
                    icon = "[D]";  // Dragon
                else if (name.find("Static") != std::string::npos)
                    icon = "[S]";  // Static

                if (isSel) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.3f, 1.0f));

                glm::vec3 pos = obj->GetTransform().GetPosition();
                ImGui::Text("%s %d: %s (%.0f,%.0f,%.0f)",
                    icon, obj->GetID(), name.c_str(), pos.x, pos.y, pos.z);

                if (isSel) ImGui::PopStyleColor();
            }

            ImGui::EndChild();
        }

        ImGui::End();
    }

    // ===========================
    // BUTTON SYSTEM
    // ===========================

    void GuiManager::AddButton(const GuiButton& button) {
        m_buttons.push_back(button);
    }

    void GuiManager::ClearButtons() {
        m_buttons.clear();
    }

    // ===========================
    // DATA BINDING
    // ===========================

    void GuiManager::BindSystems(Scene* scene, SceneManager* sceneManager, SelectionSystem* selectionSystem, TileManager* tileManager) {
        m_scene = scene;
        m_sceneManager = sceneManager;
        m_selectionSystem = selectionSystem;
        m_tileManager = tileManager;

        if (m_tileManager) {
            m_tileSize = static_cast<int>(m_tileManager->GetTileSize());
            m_tileModelScale = m_tileManager->GetTileModelScale().x;
        }
    }

    // ===========================
    // UTILITY
    // ===========================

    bool GuiManager::WantsMouseInput() const {
        if (!m_initialized) return false;
        return ImGui::GetIO().WantCaptureMouse;
    }

    bool GuiManager::WantsKeyboardInput() const {
        if (!m_initialized) return false;
        return ImGui::GetIO().WantCaptureKeyboard;
    }

    // ===========================
    // HELPER: buton colorat
    // ===========================

    bool GuiManager::ColoredButton(const char* label, ImVec2 size, ImVec4 color, ImVec4 hoverColor) {
        ImGui::PushStyleColor(ImGuiCol_Button, color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
            ImVec4(hoverColor.x * 1.1f, hoverColor.y * 1.1f, hoverColor.z * 1.1f, 1.0f));

        bool pressed = ImGui::Button(label, size);

        ImGui::PopStyleColor(3);
        return pressed;
    }

    // ===========================
    // STYLE
    // ===========================

    void GuiManager::ApplyCustomStyle() {
        ImGuiStyle& style = ImGui::GetStyle();

        style.WindowRounding = 4.0f;
        style.FrameRounding = 3.0f;
        style.GrabRounding = 3.0f;
        style.ScrollbarRounding = 3.0f;
        style.TabRounding = 3.0f;

        style.WindowPadding = ImVec2(10, 8);
        style.FramePadding = ImVec2(8, 4);
        style.ItemSpacing = ImVec2(8, 4);

        style.WindowBorderSize = 1.0f;
        style.Alpha = 0.96f;

        // Dark RTS theme — fundal inchis, accente portocalii/aurii
        ImVec4* c = style.Colors;
        c[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.07f, 0.09f, 0.92f);
        c[ImGuiCol_Border] = ImVec4(0.35f, 0.30f, 0.20f, 0.50f);
        c[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.13f, 0.12f, 1.00f);
        c[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.20f, 0.16f, 1.00f);
        c[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.26f, 0.18f, 1.00f);
        c[ImGuiCol_TitleBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
        c[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.10f, 0.06f, 1.00f);
        c[ImGuiCol_Button] = ImVec4(0.22f, 0.20f, 0.16f, 1.00f);
        c[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.30f, 0.18f, 1.00f);
        c[ImGuiCol_ButtonActive] = ImVec4(0.50f, 0.40f, 0.15f, 1.00f);
        c[ImGuiCol_Header] = ImVec4(0.22f, 0.20f, 0.14f, 1.00f);
        c[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.30f, 0.18f, 1.00f);
        c[ImGuiCol_HeaderActive] = ImVec4(0.50f, 0.40f, 0.15f, 1.00f);
        c[ImGuiCol_SeparatorHovered] = ImVec4(0.60f, 0.45f, 0.15f, 0.78f);
        c[ImGuiCol_SeparatorActive] = ImVec4(0.70f, 0.50f, 0.15f, 1.00f);
        c[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.70f, 0.20f, 1.00f);
        c[ImGuiCol_SliderGrab] = ImVec4(0.60f, 0.45f, 0.15f, 1.00f);
        c[ImGuiCol_SliderGrabActive] = ImVec4(0.80f, 0.60f, 0.20f, 1.00f);
        c[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.50f);
        c[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.25f, 0.15f, 1.00f);
        c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.33f, 0.18f, 1.00f);
        c[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.50f, 0.40f, 0.20f, 1.00f);
        c[ImGuiCol_PlotLines] = ImVec4(0.80f, 0.60f, 0.20f, 1.00f);
    }

} // namespace gps