//
// GuiManager.cpp
// GUI RTS-style: Top Bar, Command Panel, Unit Info, Debug
//

#include "GuiManager.hpp"
#include "EditorState.hpp"
#include "Scene.hpp"
#include "SceneManager.hpp"
#include "SceneObject.hpp"
#include "SelectionSystem.hpp"
#include "TileManager.hpp"
#include "ResourceManager.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>

// Oil cost for placing a prop via the spawn panel. Mirrors main.cpp; kept here
// so the spawn buttons can grey out and surface the price.
static constexpr float kPropPlacementOilCost = 50.0f;

namespace gps {

    GuiManager::GuiManager()
        : m_initialized(false)
        , m_window(nullptr)
        , m_showDebugPanel(true)
        , m_scene(nullptr)
        , m_sceneManager(nullptr)
        , m_selectionSystem(nullptr)
        , m_tileManager(nullptr)
        , m_editorState(nullptr)
        , m_tileGridSize(5)
        , m_cameraPos(0.0f)
        , m_deltaTime(0.016f)
        , m_zoomFactor(1.0f)
        , m_fpsHistoryIdx(0)
        , m_tileSize(256)
        , m_tileModelScale(27)
        , heightTile(-60.0f)
        , m_showHelp(false)
        , m_paused(false)
        , m_victory(false)
        , m_defeat(false)
        , m_minimapClickPending(false)
        , m_minimapClickWorldPoint(0.0f)
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

        if (m_editorState && m_editorState->IsEditMode()) {
            RenderInspectorPanel();
        } else {
            RenderSpawnPanel();
            RenderCommandPanel();
            RenderUnitInfoPanel();
            RenderMinimap();
        }

        if (m_showDebugPanel) RenderDebugPanel();
        if (m_showHelp)       RenderHelpOverlay();
        if (m_paused)         RenderPauseOverlay();
        if (m_victory || m_defeat) RenderGameStateOverlay();

        RenderHealthBars();
    }

    bool GuiManager::ConsumeMinimapClick(glm::vec3& outWorldPoint) {
        if (!m_minimapClickPending) return false;
        outWorldPoint = m_minimapClickWorldPoint;
        m_minimapClickPending = false;
        return true;
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

        // Mode toggle button
        if (m_editorState) {
            bool isEdit = m_editorState->IsEditMode();
            ImVec4 modeColor = isEdit
                ? ImVec4(0.2f, 0.5f, 1.0f, 1.0f)
                : ImVec4(0.2f, 0.7f, 0.2f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, modeColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                ImVec4(modeColor.x * 1.2f, modeColor.y * 1.2f, modeColor.z * 1.2f, 1.0f));
            if (ImGui::SmallButton(isEdit ? "EDIT MODE [F5]" : "PLAY MODE [F5]")) {
                m_editorState->ToggleMode();
                if (m_selectionSystem) {
                    m_selectionSystem->SetEditModeSelection(m_editorState->IsEditMode());
                    m_selectionSystem->ClearSelection();
                }
            }
            ImGui::PopStyleColor(2);
            ImGui::SameLine(0, 20);
        }

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
        ImGui::SameLine(0, 12);
        ImGui::TextColored(ImVec4(0.7f, 0.9f, 0.5f, 1.0f),
            "Fish: %.0f", gps::ResourceManager::Instance().Get("Fish"));

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
        ImGui::SetNextWindowPos(ImVec2(10, 45), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(280, 480), ImGuiCond_FirstUseEver);

        ImGui::Begin("Spawn Units", nullptr);

        const float oil  = gps::ResourceManager::Instance().Get("Oil");
        const bool  poor = oil < kPropPlacementOilCost;

        auto spawnEntry = [&](const char* label, const char* tooltip,
                              const std::string& model, const std::string& tag,
                              float scale, const std::string& displayName,
                              ImVec4 base, ImVec4 hover)
        {
            if (poor) ImGui::BeginDisabled();
            char fullLabel[96];
            snprintf(fullLabel, sizeof(fullLabel), "%s\n(%.0f Oil)", label, kPropPlacementOilCost);
            if (ColoredButton(fullLabel, ImVec2(-1, 44), base, hover)) {
                if (m_sceneManager) {
                    m_sceneManager->SetPropPlacement(model, tag, glm::vec3(scale), displayName);
                }
            }
            if (poor) ImGui::EndDisabled();
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s\nCost: %.0f Oil\nLeft-click on the map to place, right-click or X to cancel.",
                                  tooltip, kPropPlacementOilCost);
            }
        };

        const ImVec4 green     = ImVec4(0.2f, 0.45f, 0.2f, 1.0f);
        const ImVec4 greenHi   = ImVec4(0.25f, 0.6f, 0.25f, 1.0f);
        const ImVec4 red       = ImVec4(0.7f, 0.1f, 0.1f, 1.0f);

        ImGui::SeparatorText("Combat Units");
        spawnEntry("Spawn Ship",     "Light combat ship. Projectile attack, mid range.",
                   "ship", "ship", 4.5f, "Ship", green, greenHi);
        spawnEntry("Spawn Frigate",  "Frigate. Long-range projectile attacker.",
                   "frigate", "frigate", 4.5f, "Frigate", green, greenHi);
        spawnEntry("Spawn Destroyer","Destroyer. AOE splash projectile, high HP.",
                   "destroyer", "destroyer", 4.5f, "destroyer", green, greenHi);
        spawnEntry("Spawn Carrier",  "Aircraft Carrier. Orbiting plane harasser.",
                   "aircraftCarrier", "aircraftCarrier", 4.5f, "AircraftCarrier", green, greenHi);
        spawnEntry("Spawn Enemy Frigate", "Spawns an enemy-team ship for combat testing.",
                   "ship", "enemyShip", 4.5f, "EnemyShip", red, red);

        ImGui::SeparatorText("Resource Units");
        spawnEntry("Oil Rig",  "Stationary oil extractor. Produces Oil over time.",
                   "oilRig", "oilRig", 2.5f, "OilRig", green, greenHi);
        spawnEntry("Fish Boat","Fish boat. Only produces Fish while parked on a Fish tile.",
                   "fishBoat", "fishBoat", 4.5f, "FishBoat", green, greenHi);

        ImGui::SeparatorText("Buildings");
        spawnEntry("Spawn CIWS","CIWS turret. Stationary, very fast fire rate.",
                   "turret", "turret", 10.5f, "Turret", green, greenHi);

        if (poor) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.3f, 1.0f),
                "Need %.0f Oil to place.", kPropPlacementOilCost);
        }

        ImGui::End();
    }
    // ===========================
    // INSPECTOR PANEL — Edit Mode
    // ===========================

    void GuiManager::RenderInspectorPanel() {
        if (!m_editorState || !m_editorState->IsEditMode()) return;
        if (!m_scene) return;

        ImGui::SetNextWindowPos(ImVec2(10, 45), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(320, 420), ImGuiCond_FirstUseEver);

        ImGui::Begin("Inspector", nullptr);

        // Tool selector
        ImGui::SeparatorText("Tool");
        EditTool currentTool = m_editorState->GetActiveTool();
        if (ImGui::RadioButton("Translate (G)", currentTool == EditTool::Translate))
            m_editorState->SetActiveTool(EditTool::Translate);
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale (R)", currentTool == EditTool::Scale))
            m_editorState->SetActiveTool(EditTool::Scale);

        ImGui::Spacing();

        if (!m_selectionSystem || !m_selectionSystem->HasSelection()) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.0f), "No object selected.");
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.0f), "Click an object to select it.");
            ImGui::End();
            return;
        }

        const auto& selectedIDs = m_selectionSystem->GetSelectedIDs();

        if (selectedIDs.size() == 1) {
            int id = *selectedIDs.begin();
            SceneObject* obj = m_scene->GetObjectByID(id);
            if (!obj) { ImGui::End(); return; }

            // Object info
            ImGui::SeparatorText("Object");
            ImGui::Text("Name: %s", obj->GetName().c_str());
            ImGui::Text("ID: %d  |  Tag: %s", obj->GetID(), obj->GetTag().c_str());

            // Transform - Position
            ImGui::SeparatorText("Position");
            glm::vec3 pos = obj->GetTransform().GetPosition();
            if (ImGui::DragFloat3("##Pos", &pos.x, 1.0f)) {
                obj->GetTransform().SetPosition(pos);
                obj->UpdateWorldBounds();
            }

            // Transform - Rotation
            ImGui::SeparatorText("Rotation");
            glm::vec3 rot = obj->GetTransform().GetRotation();
            if (ImGui::DragFloat3("##Rot", &rot.x, 1.0f, -360.0f, 360.0f)) {
                obj->GetTransform().SetRotation(rot);
                obj->UpdateWorldBounds();
            }

            // Transform - Scale
            ImGui::SeparatorText("Scale");
            glm::vec3 scl = obj->GetTransform().GetScale();
            if (ImGui::DragFloat3("##Scl", &scl.x, 0.1f, 0.01f, 100.0f)) {
                obj->GetTransform().SetScale(scl);
                obj->UpdateWorldBounds();
            }

            // Uniform scale
            ImGui::Spacing();
            float uniScale = scl.x;
            if (ImGui::SliderFloat("Uniform Scale", &uniScale, 0.1f, 50.0f)) {
                obj->GetTransform().SetScale(uniScale);
                obj->UpdateWorldBounds();
            }

            // Single-tile type editor.
            if (m_tileManager && obj->GetTag() == "tile") {
                int gx, gz;
                m_tileManager->WorldToGrid(obj->GetTransform().GetPosition(), gx, gz);
                if (Tile* tile = m_tileManager->GetTile(gx, gz)) {
                    ImGui::SeparatorText("Tile Type");
                    static const char* kTypeNames[] = { "Sea", "Oil", "Fish", "Shallows", "Land" };
                    int current = static_cast<int>(tile->type);
                    if (ImGui::Combo("##TileType", &current, kTypeNames, IM_ARRAYSIZE(kTypeNames))) {
                        m_tileManager->SetTileType(gx, gz, static_cast<TileType>(current));
                    }
                }
            }
        }
        else {
            ImGui::SeparatorText("Multiple Selection");
            ImGui::Text("%zu objects selected", selectedIDs.size());
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.0f),
                "Select a single object to edit its transform.");

            // Multi-tile type editor: apply the chosen TileType to every
            // selected tile. Only fires the frame the combo actually changes
            // (Combo() returns true once) so we don't hammer SetTileType.
            if (m_tileManager) {
                bool anyTile = false;
                for (int id : selectedIDs) {
                    SceneObject* o = m_scene->GetObjectByID(id);
                    if (o && o->GetTag() == "tile") { anyTile = true; break; }
                }
                if (anyTile) {
                    ImGui::SeparatorText("Tile Type (Bulk)");
                    static const char* kTypeNames[] = { "Sea", "Oil", "Fish", "Shallows", "Land" };
                    static int bulkChoice = 0;
                    if (ImGui::Combo("##BulkTileType", &bulkChoice, kTypeNames, IM_ARRAYSIZE(kTypeNames))) {
                        TileType newType = static_cast<TileType>(bulkChoice);
                        for (int id : selectedIDs) {
                            SceneObject* o = m_scene->GetObjectByID(id);
                            if (!o || o->GetTag() != "tile") continue;
                            int gx, gz;
                            m_tileManager->WorldToGrid(o->GetTransform().GetPosition(), gx, gz);
                            m_tileManager->SetTileType(gx, gz, newType);
                        }
                    }
                }
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

        // ─── ABILITIES ───
        ImGui::SeparatorText("Abilities");

        if (ColoredButton("Bombardment (75 Oil)", ImVec2(-1, 40),
                ImVec4(0.8f, 0.25f, 0.1f, 1.0f), ImVec4(1.0f, 0.35f, 0.15f, 1.0f))) {
            if (m_sceneManager) m_sceneManager->m_bombardmentTargeting = true;
        }

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
                ImGui::PopStyleColor();
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.8f, 1.0f),
                    "Tag: %s   Attack: %d",
                    obj->GetTag().c_str(), obj->unitStats.attack);

                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.0f), "(ID: %d)", obj->GetID());

                ImGui::Separator();
                
            if(obj->unitStats.isCombatUnit==true)
            {
                ImGui::Text("Upgrades");

                ImGui::BulletText("Attack: %d",   obj->unitStats.attack);
                ImGui::BulletText("Range: %.0f",  obj->unitStats.attackRange);

                if (ImGui::Button("Upgrade Attack", ImVec2(120, 24))) {
                    obj->unitStats.attack += 10;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Permanently add +10 to this unit's attack damage.");

                ImGui::SameLine();

                if (ImGui::Button("Upgrade Range", ImVec2(120, 24))) {
                    obj->unitStats.attackRange += 10.0f;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Permanently add +10 to this unit's attack range.");
            }
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

                // Combat stance (combat units only)
                if (obj->unitStats.isCombatUnit) {
                    static const char* kStanceNames[] = { "Neutral", "Aggressive", "Defensive" };
                    int s = (int)obj->unitStats.stance;
                    ImGui::SetNextItemWidth(140.0f);
                    if (ImGui::Combo("Stance", &s, kStanceNames, IM_ARRAYSIZE(kStanceNames)))
                        obj->unitStats.stance = (CombatStance)s;
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
            int combatCount = 0;
            int firstStance = -1;
            bool stanceMixed = false;
            int hpSum = 0;
            int hpMaxSum = 0;
            int attackSum = 0;

            for (int id : selectedIDs) {
                SceneObject* obj = m_scene->GetObjectByID(id);
                if (!obj) continue;

                std::string name = obj->GetName();
                if (name.find("Orc") != std::string::npos) orcCount++;
                else if (name.find("Troop") != std::string::npos) troopCount++;
                else otherCount++;

                if (obj->movement.isMoving) movingCount++;

                hpSum    += obj->unitStats.health;
                hpMaxSum += obj->unitStats.maxHealth;

                if (obj->unitStats.isCombatUnit) {
                    int s = (int)obj->unitStats.stance;
                    if (firstStance == -1) firstStance = s;
                    else if (s != firstStance) stanceMixed = true;
                    combatCount++;
                    attackSum += obj->unitStats.attack;
                }
            }

            if (orcCount > 0)   ImGui::Text("  Orcs: %d", orcCount);
            if (troopCount > 0) ImGui::Text("  Troops: %d", troopCount);
            if (otherCount > 0) ImGui::Text("  Other: %d", otherCount);

            ImGui::Spacing();
            if (hpMaxSum > 0) {
                ImGui::TextColored(ImVec4(0.7f, 0.9f, 0.7f, 1.0f),
                    "Total HP: %d / %d", hpSum, hpMaxSum);
            }
            if (combatCount > 0) {
                ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.6f, 1.0f),
                    "Avg attack: %.1f (%d combat units)",
                    (float)attackSum / (float)combatCount, combatCount);
            }

            ImGui::Spacing();
            if (movingCount > 0) {
                ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f),
                    "%d moving", movingCount);
            }
            else {
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "All idle");
            }

            // Stance combo: applies to all selected combat units on change
            if (combatCount > 0) {
                static const char* kStanceNames[] = { "Neutral", "Aggressive", "Defensive" };
                int s = stanceMixed ? -1 : firstStance;
                ImGui::SetNextItemWidth(140.0f);
                const char* preview = (s >= 0 && s < 3) ? kStanceNames[s] : "Mixed";
                if (ImGui::BeginCombo("Stance", preview)) {
                    for (int i = 0; i < 3; ++i) {
                        bool selected = (s == i);
                        if (ImGui::Selectable(kStanceNames[i], selected)) {
                            for (int id : selectedIDs) {
                                SceneObject* obj = m_scene->GetObjectByID(id);
                                if (obj && obj->unitStats.isCombatUnit)
                                    obj->unitStats.stance = (CombatStance)i;
                            }
                        }
                    }
                    ImGui::EndCombo();
                }
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
        // Default to bottom-left so it doesn't fight SpawnPanel (top-left),
        // CommandPanel (bottom-right) or the new Minimap (top-right).
        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(
            ImVec2(vp->WorkPos.x + 10,
                   vp->WorkPos.y + vp->WorkSize.y - 410),
            ImGuiCond_FirstUseEver);
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

    // ===========================
    // HELP OVERLAY — H key, hotkey legend
    // ===========================

    void GuiManager::RenderHelpOverlay() {
        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImVec2 size(380, 360);
        ImGui::SetNextWindowPos(
            ImVec2(vp->WorkPos.x + (vp->WorkSize.x - size.x) * 0.5f,
                   vp->WorkPos.y + (vp->WorkSize.y - size.y) * 0.5f),
            ImGuiCond_Always);
        ImGui::SetNextWindowSize(size, ImGuiCond_Always);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.06f, 0.10f, 0.96f));
        ImGui::Begin("Help [H]", &m_showHelp,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Camera");
        ImGui::BulletText("W A S D            Pan camera");
        ImGui::BulletText("Mouse to edge      Pan camera");
        ImGui::BulletText("Middle mouse drag  Pan camera");
        ImGui::BulletText("Scroll wheel       Zoom (smoothed)");
        ImGui::BulletText("F (double-tap)     Focus on selection");

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Gameplay");
        ImGui::BulletText("B                  Spawn Orc");
        ImGui::BulletText("N                  Spawn Formation");
        ImGui::BulletText("T                  Select all troops");
        ImGui::BulletText("C                  Clear selection");
        ImGui::BulletText("X                  Cancel placement mode");
        ImGui::BulletText("P                  Pause / Resume");

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Panels & Modes");
        ImGui::BulletText("F1                 Toggle debug panel");
        ImGui::BulletText("F5                 Toggle Edit / Play mode");
        ImGui::BulletText("H                  Toggle this help overlay");
        ImGui::BulletText("ESC                Quit");

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.8f, 1.0f), "Edit Mode only:");
        ImGui::BulletText("G                  Translate tool");
        ImGui::BulletText("R                  Scale tool");

        ImGui::End();
        ImGui::PopStyleColor();
    }

    // ===========================
    // MINIMAP — top-right under TopBar, click teleports camera
    // ===========================

    void GuiManager::RenderMinimap() {
        if (!m_tileManager) return;
        glm::vec3 gMin, gMax;
        if (!m_tileManager->GetGridBounds(gMin, gMax)) return;

        ImGuiViewport* vp = ImGui::GetMainViewport();
        const float side   = 180.0f;
        const float margin = 10.0f;

        ImGui::SetNextWindowPos(
            ImVec2(vp->WorkPos.x + vp->WorkSize.x - side - margin,
                   vp->WorkPos.y + 40.0f),
            ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(side, side), ImGuiCond_Always);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2, 2));
        ImGui::Begin("Minimap", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings);

        ImVec2 origin = ImGui::GetCursorScreenPos();
        ImVec2 avail  = ImGui::GetContentRegionAvail();
        ImDrawList* dl = ImGui::GetWindowDrawList();

        const float worldW = gMax.x - gMin.x;
        const float worldH = gMax.z - gMin.z;
        if (worldW <= 0.0f || worldH <= 0.0f) { ImGui::End(); ImGui::PopStyleVar(); return; }

        // Match what the player sees on screen. The camera at (0, 200, -200)
        // makes its right vector point to world -X, so world +X is on the LEFT
        // of the screen and world +Z is at the TOP. Flip both axes so the
        // minimap reads the same way the world does on screen.
        auto worldToMini = [&](float wx, float wz) {
            float u = 1.0f - (wx - gMin.x) / worldW;
            float v = 1.0f - (wz - gMin.z) / worldH;
            return ImVec2(origin.x + u * avail.x, origin.y + v * avail.y);
        };

        // Background
        dl->AddRectFilled(origin,
            ImVec2(origin.x + avail.x, origin.y + avail.y),
            IM_COL32(15, 25, 45, 255));

        // Tiles by type
        const float tilePx = (avail.x / std::max(1.0f, worldW)) * m_tileManager->GetTileSize();
        for (const Tile* t : m_tileManager->GetAllTiles()) {
            if (!t) continue;
            ImVec2 c = worldToMini(t->worldPos.x, t->worldPos.z);
            ImU32 col = IM_COL32(30, 60, 120, 255); // Sea
            switch (t->type) {
                case TileType::Oil:      col = IM_COL32(60, 50, 40, 255);   break;
                case TileType::Fish:     col = IM_COL32(40, 130, 150, 255); break;
                case TileType::Shallows: col = IM_COL32(70, 110, 170, 255); break;
                case TileType::Land:     col = IM_COL32(90, 95, 60, 255);   break;
                default: break;
            }
            float h = tilePx * 0.5f;
            dl->AddRectFilled(ImVec2(c.x - h, c.y - h), ImVec2(c.x + h, c.y + h), col);
        }

        // Units (active scene objects with collision radius — skips projectiles)
        if (m_scene) {
            for (const auto& objPtr : m_scene->GetObjects()) {
                SceneObject* o = objPtr.get();
                if (!o || !o->IsActive()) continue;
                if (o->projectileData.isProjectile) continue;
                if (o->GetTag() == "tile" || o->GetTag() == "islandT") continue;

                ImVec2 p = worldToMini(o->GetTransform().GetPosition().x,
                                       o->GetTransform().GetPosition().z);
                ImU32 dot = IM_COL32(180, 180, 180, 255);
                switch (o->unitStats.faction) {
                    case 1: dot = IM_COL32(80, 180, 255, 255); break;
                    case 2: dot = IM_COL32(255, 90, 90, 255);  break;
                    default: break;
                }
                bool isSel = (m_selectionSystem && m_selectionSystem->IsSelected(o->GetID()));
                float r = isSel ? 3.5f : 2.0f;
                if (isSel) dl->AddCircle(p, r + 1.0f, IM_COL32(255, 255, 80, 255), 8, 1.0f);
                dl->AddCircleFilled(p, r, dot, 8);
            }
        }

        // Camera position marker (approx — just a small X)
        {
            ImVec2 p = worldToMini(m_cameraPos.x, m_cameraPos.z);
            //const float s = 5.0f;
            ImU32 col = IM_COL32(255, 220, 120, 255);
            dl->AddLine(ImVec2(p.x, p.y ), ImVec2(p.x , p.y ), col, 1.5f);
            dl->AddLine(ImVec2(p.x, p.y ), ImVec2(p.x , p.y ), col, 1.5f);
        }

        // Border
        dl->AddRect(origin,
            ImVec2(origin.x + avail.x, origin.y + avail.y),
            IM_COL32(180, 150, 70, 255), 0.0f, 0, 1.5f);

        // Click to teleport — invisible button covers the area
        ImGui::InvisibleButton("##minimap_click", avail);
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            ImVec2 mouse = ImGui::GetIO().MousePos;
            float u = (mouse.x - origin.x) / avail.x;
            float v = (mouse.y - origin.y) / avail.y;
            u = std::clamp(u, 0.0f, 1.0f);
            v = std::clamp(v, 0.0f, 1.0f);
            // Inverse of worldToMini: both u and v are flipped.
            m_minimapClickWorldPoint = glm::vec3(
                gMin.x + (1.0f - u) * worldW,
                0.0f,
                gMin.z + (1.0f - v) * worldH);
            m_minimapClickPending = true;
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    // ===========================
    // PAUSE OVERLAY — full screen tint with centered text
    // ===========================

    void GuiManager::RenderPauseOverlay() {
        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.45f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::Begin("##PauseOverlay", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoInputs);

        const char* text = "PAUSED";
        ImVec2 sz = ImGui::CalcTextSize(text);
        ImGui::SetCursorPos(ImVec2((vp->WorkSize.x - sz.x * 4.0f) * 0.5f,
                                   (vp->WorkSize.y - sz.y * 4.0f) * 0.5f));
        ImGui::SetWindowFontScale(4.0f);
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", text);
        ImGui::SetWindowFontScale(1.0f);

        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }

    // ===========================
    // GAME STATE OVERLAY — Victory / Defeat placeholders
    // ===========================

    void GuiManager::RenderGameStateOverlay() {
        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImVec2 size(360, 180);
        ImGui::SetNextWindowPos(
            ImVec2(vp->WorkPos.x + (vp->WorkSize.x - size.x) * 0.5f,
                   vp->WorkPos.y + (vp->WorkSize.y - size.y) * 0.5f),
            ImGuiCond_Always);
        ImGui::SetNextWindowSize(size, ImGuiCond_Always);

        const bool won = m_victory;
        ImVec4 accent = won ? ImVec4(0.3f, 0.9f, 0.4f, 1.0f) : ImVec4(0.95f, 0.3f, 0.3f, 1.0f);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.06f, 0.10f, 0.98f));
        ImGui::Begin("##GameState", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

        ImGui::SetWindowFontScale(2.5f);
        const char* title = won ? "VICTORY" : "DEFEAT";
        ImVec2 ts = ImGui::CalcTextSize(title);
        ImGui::SetCursorPosX((size.x - ts.x) * 0.5f);
        ImGui::TextColored(accent, "%s", title);
        ImGui::SetWindowFontScale(1.0f);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Continue", ImVec2(-1, 32))) {
            m_victory = false;
            m_defeat  = false;
        }

        ImGui::End();
        ImGui::PopStyleColor();
    }

    // ===========================
    // HEALTH BARS — billboard above each alive unit
    // ===========================
    void GuiManager::RenderHealthBars() {
        if (!m_scene) return;

        ImGuiViewport* vp = ImGui::GetMainViewport();
        const ImVec2 vpPos = vp->Pos;
        const ImVec2 vpSize = vp->Size;
        ImDrawList* dl = ImGui::GetForegroundDrawList();

        const glm::mat4 vpMat = m_proj * m_view;

        for (const auto& objPtr : m_scene->GetObjects()) {
            SceneObject* obj = objPtr.get();
            if (!obj || !obj->IsActive()) continue;
            if (obj->projectileData.isProjectile) continue;
            if (!obj->unitStats.isAlive) continue;
            if (obj->unitStats.maxHealth <= 0) continue;

            // Anchor a bit above the unit's bounding sphere so it floats overhead.
            glm::vec3 wp = obj->GetWorldCenter();
            wp.y += obj->GetWorldRadius() + 2.0f;

            glm::vec4 clip = vpMat * glm::vec4(wp, 1.0f);
            if (clip.w <= 0.0001f) continue;
            glm::vec3 ndc = glm::vec3(clip) / clip.w;
            if (ndc.x < -1.2f || ndc.x > 1.2f) continue;
            if (ndc.y < -1.2f || ndc.y > 1.2f) continue;
            if (ndc.z < -1.0f || ndc.z >  1.0f) continue;

            const float sx = vpPos.x + (ndc.x * 0.5f + 0.5f) * vpSize.x;
            const float sy = vpPos.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * vpSize.y;

            const float w = 42.0f;
            const float h = 5.0f;
            float frac = static_cast<float>(obj->unitStats.health)
                       / static_cast<float>(obj->unitStats.maxHealth);
            frac = (frac < 0.0f) ? 0.0f : (frac > 1.0f) ? 1.0f : frac;

            const ImU32 bg     = IM_COL32(20, 20, 20, 200);
            const ImU32 border = IM_COL32(0, 0, 0, 255);
            const ImU32 fg = (frac > 0.6f) ? IM_COL32(60, 200, 60, 240)
                            : (frac > 0.3f) ? IM_COL32(230, 180, 30, 240)
                                            : IM_COL32(220, 50, 50, 240);

            const ImVec2 a(sx - w * 0.5f, sy - h * 0.5f);
            const ImVec2 b(sx + w * 0.5f, sy + h * 0.5f);
            dl->AddRectFilled(a, b, bg);
            dl->AddRectFilled(a, ImVec2(a.x + w * frac, b.y), fg);
            dl->AddRect(a, b, border);
        }
    }

} // namespace gps