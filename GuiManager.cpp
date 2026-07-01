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
#include <cmath>
#include <limits>

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
        if (!m_gameStarted && !m_victory && !m_defeat) RenderStartOverlay();

        // Pre-combat grace countdown — small banner centered under the TopBar.
        if (m_graceSecRemaining > 0.0f && m_gameStarted && !m_victory && !m_defeat) {
            ImGuiViewport* vp = ImGui::GetMainViewport();
            ImVec2 size(320, 60);
            ImGui::SetNextWindowPos(
                ImVec2(vp->WorkPos.x + (vp->WorkSize.x - size.x) * 0.5f,
                       vp->WorkPos.y + 50.0f),
                ImGuiCond_Always);
            ImGui::SetNextWindowSize(size, ImGuiCond_Always);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.10f, 0.92f));
            ImGui::Begin("##Grace", nullptr,
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoInputs);
            ImGui::SetWindowFontScale(1.5f);
            char buf[48];
            snprintf(buf, sizeof(buf), "Battle starts in %d", (int)std::ceil(m_graceSecRemaining));
            ImVec2 ts = ImGui::CalcTextSize(buf);
            ImGui::SetCursorPosX((size.x - ts.x) * 0.5f);
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", buf);
            ImGui::SetWindowFontScale(1.0f);
            ImGui::End();
            ImGui::PopStyleColor();
        }

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

        // Control groups indicator: lists slots that contain something.
        if (m_selectionSystem) {
            std::string groups;
            for (int k = 0; k <= 9; ++k) {
                if (m_selectionSystem->GetGroupSize(k) > 0) {
                    if (!groups.empty()) groups += " ";
                    groups += std::to_string(k);
                }
            }
            if (!groups.empty()) {
                ImGui::SameLine(0, 20);
                ImGui::TextColored(ImVec4(0.65f, 0.85f, 1.0f, 1.0f), "Groups: %s", groups.c_str());
            }
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
        ImGui::SetNextWindowSize(ImVec2(300, 540), ImGuiCond_FirstUseEver);

        ImGui::Begin("Spawn Units", nullptr);

        const float oil  = gps::ResourceManager::Instance().Get("Oil");
        const float fish = gps::ResourceManager::Instance().Get("Fish");

        // Faction toggle: every spawnable can be placed as friendly OR enemy.
        // The faction is stored on SceneManager's prop-placement state and
        // applied to the spawned object's UnitStats in main.cpp.
        static int factionChoice = 1; // 1 = friendly, 2 = enemy
        const bool  isEnemy = (factionChoice == 2);
        const ImVec4 friendlyCol(0.10f, 0.45f, 0.85f, 1.0f);
        const ImVec4 enemyCol   (0.85f, 0.20f, 0.20f, 1.0f);
        const ImVec4 accent     = isEnemy ? enemyCol : friendlyCol;
        const ImVec4 accentHi(accent.x * 1.25f, accent.y * 1.25f, accent.z * 1.25f, 1.0f);

        // Big banner so the player can never mistake which side they're placing for.
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(accent.x * 0.35f, accent.y * 0.35f, accent.z * 0.35f, 1.0f));
        ImGui::BeginChild("##factionBanner", ImVec2(0, 56), true);
        ImGui::SetWindowFontScale(1.15f);
        ImGui::TextColored(accent, isEnemy ? "ENEMY (P2)" : "FRIENDLY (P1)");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.9f, 1.0f),
            "Choose a side, then pick a unit below.");
        ImGui::EndChild();
        ImGui::PopStyleColor();

        // Faction radio — same row, equal width.
        const float half = (ImGui::GetContentRegionAvail().x - 6.0f) * 0.5f;
        ImGui::PushStyleColor(ImGuiCol_Button,        factionChoice == 1 ? friendlyCol : ImVec4(0.18f, 0.18f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(friendlyCol.x * 1.25f, friendlyCol.y * 1.25f, friendlyCol.z * 1.25f, 1.0f));
        if (ImGui::Button("Friendly (P1)", ImVec2(half, 28))) factionChoice = 1;
        ImGui::PopStyleColor(2);
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button,        factionChoice == 2 ? enemyCol : ImVec4(0.18f, 0.18f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(enemyCol.x * 1.25f, enemyCol.y * 1.25f, enemyCol.z * 1.25f, 1.0f));
        if (ImGui::Button("Enemy (P2)", ImVec2(half, 28))) factionChoice = 2;
        ImGui::PopStyleColor(2);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        auto spawnEntry = [&](const char* label, const char* tooltip,
                              const std::string& model, const std::string& tag,
                              float scale, const std::string& displayName)
        {
            // Per-prop price (shared table) — grey the button out unless the
            // player can cover BOTH resources.
            const gps::UnitStats::PricePoints price = gps::GetPropPrice(tag);
            const bool affordable = (oil >= price.oil) && (fish >= price.fish);

            if (!affordable) ImGui::BeginDisabled();
            char fullLabel[96];
            snprintf(fullLabel, sizeof(fullLabel), "%s%s\n(%d Oil / %d Fish)",
                     isEnemy ? "[E] " : "", label, price.oil, price.fish);
            if (ColoredButton(fullLabel, ImVec2(-1, 44), accent, accentHi)) {
                if (m_sceneManager) {
                    m_sceneManager->SetPropPlacement(model, tag, glm::vec3(scale), displayName, factionChoice);
                }
            }
            if (!affordable) ImGui::EndDisabled();
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s\nFaction: %s\nCost: %d Oil / %d Fish\nLeft-click to place, right-click or X to cancel.",
                                  tooltip, isEnemy ? "Enemy (P2)" : "Friendly (P1)", price.oil, price.fish);
            }
        };

        ImGui::SeparatorText("Combat Units");
        spawnEntry("Ship",      "Light combat ship. Projectile attack, mid range.",
                   "ship", "ship", 6.5f, "Ship");
        spawnEntry("Frigate",   "Frigate. Long-range projectile attacker.",
                   "frigate", "frigate", 6.5f, "Frigate");
        spawnEntry("Destroyer", "Destroyer. AOE splash projectile, high HP.",
                   "destroyer", "destroyer", 6.5f, "Destroyer");
        spawnEntry("Carrier",   "Aircraft Carrier. Deploys an orbiting plane; respawns it on death.",
                   "aircraftCarrier", "aircraftCarrier", 6.5f, "AircraftCarrier");

        ImGui::SeparatorText("Resource Units");
        spawnEntry("Oil Rig",  "Stationary oil extractor. Produces Oil over time.",
                   "oilRig", "oilRig", 2.5f, "OilRig");
        spawnEntry("Fish Boat","Fish boat. Only produces Fish while parked on a Fish tile.",
                   "fishBoat", "fishBoat", 4.5f, "FishBoat");

        ImGui::SeparatorText("Buildings");
        spawnEntry("CIWS Turret","CIWS turret. Stationary, very fast fire rate.",
                   "turret", "turret", 15.0f, "Turret");
        spawnEntry("Naval Mine","Stationary contact mine. Detonates on enemy contact, splash damage.",
                   "bomb", "mine", 6.0f, "Mine");

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.8f, 1.0f),
            "Oil: %.0f   Fish: %.0f   (greyed-out units cost more than you have)",
            oil, fish);

        ImGui::End();
    }
    // ===========================
    // INSPECTOR PANEL — Edit Mode
    // ===========================

    void GuiManager::RenderInspectorPanel() {
        if (!m_editorState || !m_editorState->IsEditMode()) return;
        if (!m_scene) return;

        ImGui::SetNextWindowPos(ImVec2(10, 45), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(340, 520), ImGuiCond_FirstUseEver);

        ImGui::Begin("Inspector", nullptr);

        // Colored X/Y/Z drag — reset button + drag per axis. Returns true on any change.
        auto Vec3Control = [](const char* label, glm::vec3& v, float speed, float resetVal) -> bool {
            bool changed = false;
            ImGui::PushID(label);
            ImGui::TextUnformatted(label);

            const float btn = ImGui::GetFrameHeight();
            const float w   = std::max(40.0f, (ImGui::GetContentRegionAvail().x - 3 * btn - 18.0f) / 3.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));

            struct Axis { const char* lbl; float* val; ImVec4 col; ImVec4 hov; };
            Axis ax[3] = {
                { "X", &v.x, ImVec4(0.80f, 0.20f, 0.22f, 1.0f), ImVec4(0.95f, 0.30f, 0.30f, 1.0f) },
                { "Y", &v.y, ImVec4(0.25f, 0.70f, 0.25f, 1.0f), ImVec4(0.35f, 0.85f, 0.35f, 1.0f) },
                { "Z", &v.z, ImVec4(0.22f, 0.40f, 0.85f, 1.0f), ImVec4(0.32f, 0.55f, 0.98f, 1.0f) },
            };
            for (int i = 0; i < 3; ++i) {
                if (i > 0) ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button,        ax[i].col);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ax[i].hov);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ax[i].hov);
                if (ImGui::Button(ax[i].lbl, ImVec2(btn, btn))) { *ax[i].val = resetVal; changed = true; }
                ImGui::PopStyleColor(3);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(w);
                char id[8]; snprintf(id, sizeof(id), "##%s", ax[i].lbl);
                if (ImGui::DragFloat(id, ax[i].val, speed, 0.0f, 0.0f, "%.2f")) changed = true;
            }
            ImGui::PopStyleVar();
            ImGui::PopID();
            return changed;
        };

        // Tile-type combo helper. Sets `outTile` to first selected tile for header context.
        auto TileTypeCombo = [&](const char* labelId, int& current) -> bool {
            static const char* kTypeNames[] = { "Sea", "Oil", "Fish", "Shallows", "Land" };
            ImGui::SetNextItemWidth(-1);
            return ImGui::Combo(labelId, &current, kTypeNames, IM_ARRAYSIZE(kTypeNames));
        };

        // ─── Tool selector ───
        ImGui::SeparatorText("Tool");
        EditTool currentTool = m_editorState->GetActiveTool();
        if (ImGui::RadioButton("Translate (G)", currentTool == EditTool::Translate))
            m_editorState->SetActiveTool(EditTool::Translate);
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale (R)", currentTool == EditTool::Scale))
            m_editorState->SetActiveTool(EditTool::Scale);

        ImGui::Spacing();

        if (!m_selectionSystem || !m_selectionSystem->HasSelection()) {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.10f, 0.14f, 1.0f));
            ImGui::BeginChild("##empty", ImVec2(0, 80), true);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 18.0f);
            ImGui::SetWindowFontScale(1.1f);
            const char* msg = "Nothing selected";
            float tw = ImGui::CalcTextSize(msg).x;
            ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - tw) * 0.5f);
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.8f, 1.0f), "%s", msg);
            ImGui::SetWindowFontScale(1.0f);
            ImGui::Spacing();
            const char* hint = "Click an object or drag a box to select.";
            tw = ImGui::CalcTextSize(hint).x;
            ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - tw) * 0.5f);
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.0f), "%s", hint);
            ImGui::EndChild();
            ImGui::PopStyleColor();
            ImGui::End();
            return;
        }

        const auto& selectedIDs = m_selectionSystem->GetSelectedIDs();
        const size_t selCount   = selectedIDs.size();

        // ─── Header card (object name / "N selected") ───
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.10f, 0.07f, 1.0f));
        ImGui::BeginChild("##header", ImVec2(0, 56), true);
        if (selCount == 1) {
            SceneObject* obj = m_scene->GetObjectByID(*selectedIDs.begin());
            if (obj) {
                ImGui::SetWindowFontScale(1.25f);
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", obj->GetName().c_str());
                ImGui::SetWindowFontScale(1.0f);
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.8f, 1.0f),
                    "ID %d   |   Tag: %s", obj->GetID(), obj->GetTag().c_str());
            }
        } else {
            ImGui::SetWindowFontScale(1.25f);
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%zu objects selected", selCount);
            ImGui::SetWindowFontScale(1.0f);
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.8f, 1.0f),
                "Transforms apply to the whole group.");
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::Spacing();

        // ─── Transform editing (works for single AND multi-select) ───
        // Multi-select strategy: drag the centroid/first values; apply the delta
        // (or multiplicative ratio for scale) to every selected object.
        glm::vec3 centroid(0.0f);
        glm::vec3 refRot(0.0f), refScl(1.0f);
        int n = 0;
        SceneObject* firstObj = nullptr;
        for (int id : selectedIDs) {
            SceneObject* o = m_scene->GetObjectByID(id);
            if (!o) continue;
            centroid += o->GetTransform().GetPosition();
            if (!firstObj) {
                firstObj = o;
                refRot   = o->GetTransform().GetRotation();
                refScl   = o->GetTransform().GetScale();
            }
            n++;
        }
        if (n == 0) { ImGui::End(); return; }
        centroid /= static_cast<float>(n);

        auto applyToAll = [&](auto fn) {
            for (int id : selectedIDs) {
                SceneObject* o = m_scene->GetObjectByID(id);
                if (!o) continue;
                fn(o);
                o->UpdateWorldBounds();
            }
        };

        ImGui::SeparatorText("Transform");

        // Position — drag centroid, apply additive delta to every selected.
        glm::vec3 newCentroid = centroid;
        if (Vec3Control("Position", newCentroid, 1.0f, 0.0f)) {
            glm::vec3 delta = newCentroid - centroid;
            applyToAll([&](SceneObject* o) { o->GetTransform().Translate(delta); });
        }

        // Rotation — drag first object's rotation, apply additive delta to all.
        glm::vec3 newRot = refRot;
        if (Vec3Control("Rotation", newRot, 1.0f, 0.0f)) {
            glm::vec3 delta = newRot - refRot;
            applyToAll([&](SceneObject* o) { o->GetTransform().Rotate(delta); });
        }

        // Scale — multiplicative ratio so units with different starting scales
        // keep their relative size when the group is scaled.
        glm::vec3 newScl = refScl;
        if (Vec3Control("Scale", newScl, 0.05f, 1.0f)) {
            glm::vec3 ratio(
                refScl.x > 0.001f ? newScl.x / refScl.x : 1.0f,
                refScl.y > 0.001f ? newScl.y / refScl.y : 1.0f,
                refScl.z > 0.001f ? newScl.z / refScl.z : 1.0f);
            applyToAll([&](SceneObject* o) {
                glm::vec3 cur = o->GetTransform().GetScale();
                o->GetTransform().SetScale(cur * ratio);
            });
        }

        // Uniform scale slider — multiplies every selected object's scale.
        ImGui::Spacing();
        float uni = refScl.x;
        if (ImGui::SliderFloat("Uniform Scale", &uni, 0.1f, 50.0f)) {
            float ratio = (refScl.x > 0.001f) ? (uni / refScl.x) : 1.0f;
            applyToAll([&](SceneObject* o) {
                o->GetTransform().SetScale(o->GetTransform().GetScale() * ratio);
            });
        }

        // ─── Tile type (single or bulk; same combo, applied to every selected tile) ───
        if (m_tileManager) {
            std::vector<int> tileIDs;
            for (int id : selectedIDs) {
                SceneObject* o = m_scene->GetObjectByID(id);
                if (o && o->GetTag() == "tile") tileIDs.push_back(id);
            }
            if (!tileIDs.empty()) {
                ImGui::SeparatorText(tileIDs.size() == 1 ? "Tile Type" : "Tile Type (Bulk)");
                // Seed combo with the first selected tile's type so it isn't visually stuck.
                int current = 0;
                if (SceneObject* first = m_scene->GetObjectByID(tileIDs.front())) {
                    int gx, gz;
                    m_tileManager->WorldToGrid(first->GetTransform().GetPosition(), gx, gz);
                    if (Tile* t = m_tileManager->GetTile(gx, gz)) current = static_cast<int>(t->type);
                }
                if (TileTypeCombo("##TileType", current)) {
                    TileType newType = static_cast<TileType>(current);
                    for (int id : tileIDs) {
                        SceneObject* o = m_scene->GetObjectByID(id);
                        if (!o) continue;
                        int gx, gz;
                        m_tileManager->WorldToGrid(o->GetTransform().GetPosition(), gx, gz);
                        m_tileManager->SetTileType(gx, gz, newType);
                    }
                }
            }
        }

        // ─── Quick stats footer for multi-select (faction counts, etc.) ───
        if (selCount > 1) {
            ImGui::Spacing();
            ImGui::SeparatorText("Selection");
            int p1 = 0, p2 = 0, neutral = 0, tiles = 0;
            for (int id : selectedIDs) {
                SceneObject* o = m_scene->GetObjectByID(id);
                if (!o) continue;
                if (o->GetTag() == "tile") { tiles++; continue; }
                switch (o->unitStats.faction) {
                    case 1:  p1++;      break;
                    case 2:  p2++;      break;
                    default: neutral++; break;
                }
            }
            if (p1)      ImGui::TextColored(ImVec4(0.3f, 0.7f, 1.0f, 1.0f), "P1 units: %d", p1);
            if (p2)      ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "P2 units: %d", p2);
            if (neutral) ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Neutral:  %d", neutral);
            if (tiles)   ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.6f, 1.0f), "Tiles:    %d", tiles);
        }

        ImGui::End();
    }

    // ===========================
    // COMMAND PANEL — jos-dreapta, butoane de actiune
    // ===========================

    void GuiManager::RenderCommandPanel() {
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        float panelWidth = 300.0f;
        float panelHeight = 600.0f;
        float margin = 10.0f;

        ImGui::SetNextWindowPos(
            ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - panelWidth - margin,
                viewport->WorkPos.y + viewport->WorkSize.y - panelHeight - margin),
            ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight), ImGuiCond_FirstUseEver);

        ImGui::Begin("Commands", nullptr, ImGuiWindowFlags_NoCollapse);

        const float btnWidth = (ImGui::GetContentRegionAvail().x - 6.0f) * 0.5f;

        // ─── HEADER CARD ───
        const ImVec4 accent(0.85f, 0.55f, 0.15f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(accent.x * 0.30f, accent.y * 0.30f, accent.z * 0.30f, 1.0f));
        ImGui::BeginChild("##cmdBanner", ImVec2(0, 50), true);
        ImGui::SetWindowFontScale(1.20f);
        ImGui::TextColored(accent, "COMMANDS");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.9f, 1.0f),
            "Selection, abilities, and map tools.");
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::Spacing();

        // ─── SELECTION ───
        ImGui::SeparatorText("Selection");

        const ImVec4 selCol  (0.18f, 0.35f, 0.55f, 1.0f);
        const ImVec4 selColHi(0.25f, 0.48f, 0.75f, 1.0f);
        if (ColoredButton("Select All\nTroops [T]", ImVec2(btnWidth, 40), selCol, selColHi)) {
            if (m_selectionSystem && m_scene) {
                m_selectionSystem->SelectAllOfType(*m_scene, "Troop");
                m_selectionSystem->SelectAllOfType(*m_scene, "Orc");
                m_selectionSystem->SelectAllOfType(*m_scene, "Ship");
            }
        }
        ImGui::SameLine();
        const ImVec4 clrCol  (0.45f, 0.20f, 0.20f, 1.0f);
        const ImVec4 clrColHi(0.65f, 0.30f, 0.30f, 1.0f);
        if (ColoredButton("Clear\nSelection [C]", ImVec2(btnWidth, 40), clrCol, clrColHi)) {
            if (m_selectionSystem) m_selectionSystem->ClearSelection();
        }

        // ─── ABILITIES ───
        ImGui::SeparatorText("Abilities");

        const ImVec4 bombCol  (0.80f, 0.25f, 0.10f, 1.0f);
        const ImVec4 bombColHi(1.00f, 0.35f, 0.15f, 1.0f);
        if (ColoredButton("Bombardment\n(75 Oil)", ImVec2(-1, 44), bombCol, bombColHi)) {
            if (m_sceneManager) m_sceneManager->m_bombardmentTargeting = true;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("8 explosive shells in a dispersion pattern. Click target on map.");

        // ─── QUICK SPAWN (legacy hotkey buttons; spawn panel handles placement) ───
        if (ImGui::CollapsingHeader("Quick Spawn (legacy)")) {
            ImGui::PushID("legacySpawn");
            const ImVec4 troopCol  (0.20f, 0.45f, 0.20f, 1.0f);
            const ImVec4 troopColHi(0.25f, 0.60f, 0.25f, 1.0f);

            if (ColoredButton("Spawn Troop [B]", ImVec2(-1, 28), troopCol, troopColHi)) {
                if (m_sceneManager) m_sceneManager->SpawnTroop(m_sceneManager->GetTroopSpawnPosition());
            }
            ImGui::SliderInt("Count",   &spawnFormationCount,   2, 30);
            ImGui::SliderFloat("Spacing", &spawnFormationSpacing, 5.0f, 40.0f, "%.0f");
            if (ColoredButton("Spawn Formation [N]", ImVec2(-1, 28),
                              ImVec4(0.20f, 0.30f, 0.50f, 1.0f), ImVec4(0.25f, 0.40f, 0.65f, 1.0f))) {
                if (m_sceneManager) {
                    glm::vec3 pos(100.0f, -60.0f, -90.0f);
                    m_sceneManager->SpawnTroopFormation(pos, spawnFormationCount, spawnFormationSpacing);
                }
            }
            ImGui::PopID();
        }

        // ─── MAP / TILE GRID (collapsed by default; advanced controls) ───
        if (m_tileManager && ImGui::CollapsingHeader("Map / Tile Grid")) {
            ImGui::PushID("mapGrid");
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.8f, 1.0f),
                "Loaded: %d / %d   |   Tile size: %.0f",
                m_tileManager->GetLoadedCount(), m_tileManager->GetTotalCount(),
                m_tileManager->GetTileSize());

            ImGui::SliderInt("Grid (NxN)", &m_tileGridSize, 1, 7);

            const ImVec4 genCol  (0.30f, 0.35f, 0.20f, 1.0f);
            const ImVec4 genColHi(0.40f, 0.50f, 0.25f, 1.0f);
            if (ColoredButton("Generate Grid", ImVec2(btnWidth, 30), genCol, genColHi)) {
                int half = m_tileGridSize / 2;
                m_tileManager->Clear();
                m_tileManager->GenerateAndLoadGrid(-half, half, -half, half);
            }
            ImGui::SameLine();
            if (ColoredButton("Clear Tiles", ImVec2(btnWidth, 30), clrCol, clrColHi)) {
                m_tileManager->Clear();
            }

            // Single-tile add/remove
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.7f, 1.0f), "Single tile:");
            static int tileX = 0, tileZ = 0;
            ImGui::PushItemWidth(btnWidth - 20);
            ImGui::InputInt("X", &tileX); ImGui::SameLine();
            ImGui::InputInt("Z", &tileZ);
            ImGui::PopItemWidth();
            if (ColoredButton("Add", ImVec2(btnWidth, 26), genCol, genColHi))
                m_tileManager->LoadTile(tileX, tileZ);
            ImGui::SameLine();
            if (ColoredButton("Remove", ImVec2(btnWidth, 26), clrCol, clrColHi))
                m_tileManager->UnloadTile(tileX, tileZ);

            // Resize
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.7f, 1.0f), "Resize:");
            ImGui::InputInt("Grid Tile Size",  &m_tileSize);
            ImGui::InputInt("Tile Model Scale",&m_tileModelScale);
            ImGui::InputInt("Tile Height",     &heightTile);
            if (ColoredButton("Apply Resize", ImVec2(-1, 28), genCol, genColHi)) {
                bool hadTiles = (m_tileManager->GetTotalCount() > 0);
                int minX = 0, maxX = 0, minZ = 0, maxZ = 0;
                if (hadTiles) m_tileManager->GetGridRange(minX, maxX, minZ, maxZ);
                m_tileManager->SetTileSize(static_cast<float>(m_tileSize));
                m_tileManager->SetTileModelScale(glm::vec3(m_tileModelScale));
                if (hadTiles) {
                    m_tileManager->Clear();
                    m_tileManager->GenerateAndLoadGrid(minX, maxX, minZ, maxZ);
                }
                m_tileManager->SetTileHeight(heightTile);
            }
            ImGui::PopID();
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

        float panelWidth = 380.0f;
        float panelHeight = 215.0f;
        float margin = 10.0f;

        // Centered along the bottom edge.
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

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.06f, 0.10f, 0.94f));
        ImGui::Begin("Unit Info", nullptr, flags);

        // Faction → colored accent shared between header tint, HP bar, badge.
        auto factionColor = [](int f) -> ImVec4 {
            switch (f) {
                case 1:  return ImVec4(0.20f, 0.55f, 1.00f, 1.0f); // blue — player
                case 2:  return ImVec4(0.95f, 0.30f, 0.30f, 1.0f); // red  — enemy
                default: return ImVec4(0.80f, 0.80f, 0.85f, 1.0f); // grey — neutral
            }
        };
        auto factionLabel = [](int f) -> const char* {
            switch (f) {
                case 1:  return "P1";
                case 2:  return "P2";
                default: return "Neutral";
            }
        };

        const auto& selectedIDs = m_selectionSystem->GetSelectedIDs();
        size_t count = selectedIDs.size();

        // Shared patrol-action row: enters patrol targeting (snapshotting the
        // current selection) or stops any active patrol on selected units.
        // Used by both single and multi-select branches below.
        auto renderPatrolActions = [&]() {
            // Skip the row entirely if no selected unit can patrol — e.g. only
            // bases / immobile buildings are selected.
            int movableCount = 0, patrollingCount = 0;
            for (int id : selectedIDs) {
                SceneObject* o = m_scene->GetObjectByID(id);
                if (!o) continue;
                if (o->unitStats.isMovable) movableCount++;
                if (o->patrolData.isPatrolling) patrollingCount++;
            }
            if (movableCount == 0 && patrollingCount == 0) return;

            const ImVec4 patrolCol  (0.18f, 0.45f, 0.75f, 1.0f);
            const ImVec4 patrolColHi(0.25f, 0.58f, 0.95f, 1.0f);
            const ImVec4 stopCol    (0.55f, 0.20f, 0.20f, 1.0f);
            const ImVec4 stopColHi  (0.75f, 0.30f, 0.30f, 1.0f);

            const float half = (ImGui::GetContentRegionAvail().x - 6.0f) * 0.5f;
            if (movableCount > 0) {
                if (ColoredButton("Patrol [Q]", ImVec2(half, 26), patrolCol, patrolColHi)) {
                    if (m_sceneManager) {
                        m_sceneManager->m_patrolUnitIDs.clear();
                        for (int id : selectedIDs)
                            m_sceneManager->m_patrolUnitIDs.push_back(id);
                        m_sceneManager->m_patrolTargeting  = true;
                        m_sceneManager->m_patrolClickPhase = 0;
                    }
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Click two points: A then B. Ships bounce A<->B until stopped.");
                ImGui::SameLine();
            }
            if (ColoredButton("Stop Patrol", ImVec2(movableCount > 0 ? half : -1, 26),
                              stopCol, stopColHi)) {
                if (m_sceneManager) {
                    m_sceneManager->m_patrolTargeting  = false;
                    m_sceneManager->m_patrolClickPhase = 0;
                    m_sceneManager->m_patrolUnitIDs.clear();
                }
                for (int id : selectedIDs)
                    if (SceneObject* o = m_scene->GetObjectByID(id))
                        o->patrolData.isPatrolling = false;
            }

            // Live banner while patrol targeting is active.
            if (m_sceneManager && m_sceneManager->m_patrolTargeting) {
                ImGui::TextColored(ImVec4(0.5f, 0.85f, 1.0f, 1.0f),
                    m_sceneManager->m_patrolClickPhase == 0
                        ? "Click point A on the map"
                        : "Click point B on the map");
            }
        };

        if (count == 1) {
            // ─── SINGLE UNIT SELECTED ───
            int id = *selectedIDs.begin();
            SceneObject* obj = m_scene->GetObjectByID(id);

            // Belt-and-suspenders: a base destroyed mid-frame might still be
            // in the selection set until cleanup runs. Skip rendering instead
            // of dereferencing stale state.
            if (obj && (!obj->IsActive() || !obj->unitStats.isAlive)) {
                ImGui::End();
                ImGui::PopStyleColor();
                return;
            }

            if (obj) {
                const ImVec4 col = factionColor(obj->unitStats.faction);

                // Header: faction-tinted card with big name + side badge.
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(col.x * 0.22f, col.y * 0.22f, col.z * 0.22f, 1.0f));
                ImGui::BeginChild("##unitHdr", ImVec2(0, 50), true);
                ImGui::SetWindowFontScale(1.3f);
                ImGui::TextColored(ImVec4(1.0f, 0.92f, 0.5f, 1.0f), "%s", obj->GetName().c_str());
                ImGui::SetWindowFontScale(1.0f);
                ImGui::TextColored(col, "[%s]", factionLabel(obj->unitStats.faction));
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.75f, 1.0f),
                    "  %s  |  ID %d", obj->GetTag().c_str(), obj->GetID());
                ImGui::EndChild();
                ImGui::PopStyleColor();

                // HP bar — big, color tied to fraction.
                if (obj->unitStats.maxHealth > 0) {
                    float frac = (float)obj->unitStats.health / (float)obj->unitStats.maxHealth;
                    ImVec4 barColor = (frac > 0.6f) ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) :
                                     (frac > 0.3f) ? ImVec4(1.0f, 0.7f, 0.1f, 1.0f) :
                                                     ImVec4(0.9f, 0.15f, 0.15f, 1.0f);
                    char hpLabel[32];
                    snprintf(hpLabel, sizeof(hpLabel), "HP  %d / %d",
                        obj->unitStats.health, obj->unitStats.maxHealth);
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
                    ImGui::ProgressBar(frac, ImVec2(-1.0f, 22.0f), hpLabel);
                    ImGui::PopStyleColor();
                }

                // Inline stat strip — colored, short. Combat = ATK / RNG, builder = prod.
                if (obj->unitStats.isCombatUnit) {
                    ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.25f, 1.0f), "ATK %d", obj->unitStats.attack);
                    ImGui::SameLine(0, 18);
                    ImGui::TextColored(ImVec4(0.45f, 0.85f, 1.0f, 1.0f), "RNG %.0f", obj->unitStats.attackRange);
                    if (obj->unitStats.splashRadius > 0.0f) {
                        ImGui::SameLine(0, 18);
                        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f),
                            "AOE %.0f", obj->unitStats.splashRadius);
                    }
                } else if (obj->unitStats.productionRate > 0.0f) {
                    ImGui::TextColored(ImVec4(0.8f, 0.9f, 0.4f, 1.0f),
                        "Producing %.1f %s/s",
                        obj->unitStats.productionRate, obj->unitStats.resourceType.c_str());
                }

                // Movement / idle / patrol status — single line.
                ImGui::SameLine(0, 18);
                if (obj->patrolData.isPatrolling) {
                    ImGui::TextColored(ImVec4(0.5f, 0.85f, 1.0f, 1.0f),
                        "Patrolling A<->%c", obj->patrolData.headingToB ? 'B' : 'A');
                } else if (obj->movement.isMoving) {
                    float now = static_cast<float>(glfwGetTime());
                    float elapsed = now - obj->movement.moveStartTime;
                    float progress = std::min(elapsed / obj->movement.moveDuration, 1.0f);
                    ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Moving (%.0f%%)", progress * 100.0f);
                } else {
                    ImGui::TextColored(ImVec4(0.5f, 0.85f, 0.55f, 1.0f), "Idle");
                }

                // Compact action bar — stance combo + upgrades on the same row.
                if (obj->unitStats.isCombatUnit) {
                    ImGui::Spacing();
                    static const char* kStanceNames[] = { "Neutral", "Aggressive", "Defensive" };
                    int s = (int)obj->unitStats.stance;
                    ImGui::SetNextItemWidth(130.0f);
                    if (ImGui::Combo("##Stance", &s, kStanceNames, IM_ARRAYSIZE(kStanceNames)))
                        obj->unitStats.stance = (CombatStance)s;
                    ImGui::SameLine();
                    if (ImGui::Button("+10 ATK", ImVec2(80, 22))) obj->unitStats.attack += 10;
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Permanent +10 attack damage.");
                    ImGui::SameLine();
                    if (ImGui::Button("+10 RNG", ImVec2(80, 22))) obj->unitStats.attackRange += 10.0f;
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Permanent +10 attack range.");
                }

                // Patrol controls (attached to the unit card for movable units).
                renderPatrolActions();
            }
        }
        else {
            // ─── MULTIPLE UNITS SELECTED ───
            int p1 = 0, p2 = 0, neutralC = 0;
            int movingCount = 0, combatCount = 0;
            int firstStance = -1; bool stanceMixed = false;
            int hpSum = 0, hpMaxSum = 0, attackSum = 0;

            for (int id : selectedIDs) {
                SceneObject* obj = m_scene->GetObjectByID(id);
                if (!obj) continue;
                if      (obj->unitStats.faction == 1) p1++;
                else if (obj->unitStats.faction == 2) p2++;
                else                                  neutralC++;
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

            // Dominant faction drives the header tint.
            const int   dominant = (p1 >= p2 && p1 >= neutralC) ? 1
                                 : (p2 >= neutralC)             ? 2 : 0;
            const ImVec4 col     = factionColor(dominant);

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(col.x * 0.22f, col.y * 0.22f, col.z * 0.22f, 1.0f));
            ImGui::BeginChild("##unitHdrMulti", ImVec2(0, 50), true);
            ImGui::SetWindowFontScale(1.3f);
            ImGui::TextColored(ImVec4(1.0f, 0.92f, 0.5f, 1.0f), "%zu units selected", count);
            ImGui::SetWindowFontScale(1.0f);
            // Per-side counts in same colors as the single-unit faction badge.
            if (p1)       { ImGui::TextColored(factionColor(1), "P1: %d", p1); ImGui::SameLine(0, 12); }
            if (p2)       { ImGui::TextColored(factionColor(2), "P2: %d", p2); ImGui::SameLine(0, 12); }
            if (neutralC) { ImGui::TextColored(factionColor(0), "N: %d", neutralC); ImGui::SameLine(0, 12); }
            ImGui::NewLine();
            ImGui::EndChild();
            ImGui::PopStyleColor();

            // Group HP bar — same color scheme as the single-unit case.
            if (hpMaxSum > 0) {
                float frac = (float)hpSum / (float)hpMaxSum;
                ImVec4 barColor = (frac > 0.6f) ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) :
                                 (frac > 0.3f) ? ImVec4(1.0f, 0.7f, 0.1f, 1.0f) :
                                                 ImVec4(0.9f, 0.15f, 0.15f, 1.0f);
                char hpLabel[40];
                snprintf(hpLabel, sizeof(hpLabel), "Group HP  %d / %d", hpSum, hpMaxSum);
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
                ImGui::ProgressBar(frac, ImVec2(-1.0f, 22.0f), hpLabel);
                ImGui::PopStyleColor();
            }

            // Combat / movement summary on one line.
            if (combatCount > 0) {
                ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.25f, 1.0f),
                    "Avg ATK %.1f", (float)attackSum / (float)combatCount);
                ImGui::SameLine(0, 18);
            }
            ImGui::TextColored(movingCount > 0 ? ImVec4(0.3f, 0.8f, 1.0f, 1.0f)
                                               : ImVec4(0.5f, 0.85f, 0.55f, 1.0f),
                movingCount > 0 ? "%d moving" : "All idle", movingCount);

            // Bulk stance — single combo applies to every selected combat unit.
            if (combatCount > 0) {
                ImGui::Spacing();
                static const char* kStanceNames[] = { "Neutral", "Aggressive", "Defensive" };
                int s = stanceMixed ? -1 : firstStance;
                const char* preview = (s >= 0 && s < 3) ? kStanceNames[s] : "Mixed";
                ImGui::SetNextItemWidth(130.0f);
                if (ImGui::BeginCombo("##StanceMulti", preview)) {
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
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.75f, 1.0f), "Stance");
            }

            // Patrol controls — single button applies to the whole group.
            renderPatrolActions();

            // Compact ID list (debug, kept short).
            std::string idList;
            int shown = 0;
            for (int id : selectedIDs) {
                if (!idList.empty()) idList += ", ";
                idList += std::to_string(id);
                if (++shown >= 8) { idList += "..."; break; }
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
        // CommandPanel (bottom-right) or the Minimap (top-right).
        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(
            ImVec2(vp->WorkPos.x + 10,
                   vp->WorkPos.y + vp->WorkSize.y - 470),
            ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(320, 460), ImGuiCond_FirstUseEver);

        ImGui::Begin("Debug [F1]", &m_showDebugPanel);

        // ─── HEADER CARD ───
        const ImVec4 accent(0.55f, 0.85f, 0.45f, 1.0f); // green = "diagnostics"
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(accent.x * 0.22f, accent.y * 0.22f, accent.z * 0.22f, 1.0f));
        ImGui::BeginChild("##dbgBanner", ImVec2(0, 50), true);
        ImGui::SetWindowFontScale(1.2f);
        ImGui::TextColored(accent, "DIAGNOSTICS");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::TextColored(ImVec4(0.75f, 0.75f, 0.85f, 1.0f),
            "F1 to toggle.  Performance, rendering, scene.");
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::Spacing();

        // ─── PERFORMANCE ───
        ImGui::SeparatorText("Performance");

        const float fps = (m_deltaTime > 0.0f) ? (1.0f / m_deltaTime) : 0.0f;
        const ImVec4 fpsColor = (fps >= 55.0f) ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f)
                              : (fps >= 30.0f) ? ImVec4(1.0f, 0.9f, 0.3f, 1.0f)
                                               : ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
        ImGui::SetWindowFontScale(1.4f);
        ImGui::TextColored(fpsColor, "%.0f FPS", fps);
        ImGui::SetWindowFontScale(1.0f);
        ImGui::SameLine(0, 16);
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.8f, 1.0f), "%.2f ms", m_deltaTime * 1000.0f);

        // Compute rolling min/avg/max from the history ring so the graph has context.
        float fpsMin = std::numeric_limits<float>::max();
        float fpsMax = 0.0f, fpsSum = 0.0f;
        int   fpsCount = 0;
        for (float v : m_fpsHistory) {
            if (v <= 0.0f) continue;
            fpsMin = std::min(fpsMin, v);
            fpsMax = std::max(fpsMax, v);
            fpsSum += v; fpsCount++;
        }
        const float fpsAvg = (fpsCount > 0) ? (fpsSum / fpsCount) : 0.0f;
        if (fpsCount > 0) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.7f, 1.0f),
                "min %.0f  /  avg %.0f  /  max %.0f", fpsMin, fpsAvg, fpsMax);
        }
        ImGui::PlotLines("##FPSGraph", m_fpsHistory, 120, m_fpsHistoryIdx,
            nullptr, 0.0f, 120.0f, ImVec2(-1, 40));

        // ─── RENDERING TOGGLES ───
        ImGui::SeparatorText("Rendering");

        if (ImGui::Checkbox("Wireframe", &isWireframeEnabled))
            glPolygonMode(GL_FRONT_AND_BACK, isWireframeEnabled ? GL_LINE : GL_FILL);
        ImGui::SameLine(0, 18);
        ImGui::Checkbox("Collision", &showCollisionBoxes);
        ImGui::SameLine(0, 18);
        ImGui::Checkbox("Bounds",    &showBoundingSpheres);

        // ─── SCENE STATS ───
        ImGui::SeparatorText("Scene");
        if (m_scene) {
            // Per-faction + per-category counters help diagnose "where did all
            // my units go" without scrolling through the full object list.
            int p1 = 0, p2 = 0, neutral = 0;
            int tiles = 0, mines = 0, projectiles = 0, patrolling = 0, moving = 0;
            for (const auto& objPtr : m_scene->GetObjects()) {
                SceneObject* o = objPtr.get();
                if (!o) continue;
                const std::string& tag = o->GetTag();
                if (tag == "tile" || tag == "tileBorder") { tiles++; continue; }
                if (o->projectileData.isProjectile)       { projectiles++; continue; }
                if (tag == "mine")                          mines++;
                if (o->patrolData.isPatrolling)             patrolling++;
                if (o->movement.isMoving)                   moving++;
                switch (o->unitStats.faction) {
                    case 1:  p1++;      break;
                    case 2:  p2++;      break;
                    default: neutral++; break;
                }
            }
            ImGui::TextColored(ImVec4(0.20f, 0.55f, 1.0f, 1.0f), "P1: %d", p1);
            ImGui::SameLine(0, 16);
            ImGui::TextColored(ImVec4(0.95f, 0.30f, 0.30f, 1.0f), "P2: %d", p2);
            ImGui::SameLine(0, 16);
            ImGui::TextColored(ImVec4(0.80f, 0.80f, 0.85f, 1.0f), "N: %d", neutral);
            ImGui::SameLine(0, 16);
            ImGui::TextColored(ImVec4(0.60f, 0.90f, 0.60f, 1.0f), "Tiles: %d", tiles);

            ImGui::TextColored(ImVec4(0.85f, 0.55f, 0.15f, 1.0f), "Mines: %d", mines);
            ImGui::SameLine(0, 16);
            ImGui::TextColored(ImVec4(0.5f, 0.85f, 1.0f, 1.0f), "Patrol: %d", patrolling);
            ImGui::SameLine(0, 16);
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Moving: %d", moving);
            ImGui::SameLine(0, 16);
            ImGui::TextColored(ImVec4(0.7f, 0.5f, 1.0f, 1.0f), "Proj: %d", projectiles);

            // ─── OBJECT LIST ───
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.7f, 1.0f),
                "Total objects: %zu", m_scene->GetObjectCount());

            static char filterBuf[64] = "";
            ImGui::SetNextItemWidth(-1);
            ImGui::InputTextWithHint("##objFilter", "Filter by name or tag...", filterBuf, sizeof(filterBuf));
            static bool hideTiles = true;
            ImGui::Checkbox("Hide tiles", &hideTiles);

            const std::string filter(filterBuf);
            ImGui::BeginChild("SceneObjects", ImVec2(0, 130), true);
            for (const auto& objPtr : m_scene->GetObjects()) {
                SceneObject* obj = objPtr.get();
                if (!obj) continue;
                const std::string& tag = obj->GetTag();
                if (hideTiles && (tag == "tile" || tag == "tileBorder")) continue;
                const std::string& name = obj->GetName();
                if (!filter.empty()
                    && name.find(filter) == std::string::npos
                    && tag.find(filter)  == std::string::npos) continue;

                bool isSel = m_selectionSystem && m_selectionSystem->IsSelected(obj->GetID());
                ImVec4 rowColor = (obj->unitStats.faction == 1) ? ImVec4(0.55f, 0.80f, 1.0f, 1.0f)
                                : (obj->unitStats.faction == 2) ? ImVec4(1.0f,  0.55f, 0.55f, 1.0f)
                                : ImVec4(0.80f, 0.80f, 0.85f, 1.0f);
                if (isSel) rowColor = ImVec4(1.0f, 1.0f, 0.3f, 1.0f);

                glm::vec3 pos = obj->GetTransform().GetPosition();
                ImGui::TextColored(rowColor, "#%d %s  [%s]  (%.0f,%.0f)",
                    obj->GetID(), name.c_str(), tag.c_str(), pos.x, pos.z);
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

    bool GuiManager::WantsTextInput() const {
        if (!m_initialized) return false;
        return ImGui::GetIO().WantTextInput;
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
        ImGui::BulletText("Ctrl+0..9          Save selection as control group");
        ImGui::BulletText("0..9               Recall control group (Shift+N to append)");
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
        const char* title = won ? "PLAYER 1 WINS" : "PLAYER 2 WINS";
        ImVec2 ts = ImGui::CalcTextSize(title);
        ImGui::SetCursorPosX((size.x - ts.x) * 0.5f);
        ImGui::TextColored(accent, "%s", title);
        ImGui::SetWindowFontScale(1.0f);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        const int mins = static_cast<int>(m_matchElapsed) / 60;
        const int secs = static_cast<int>(m_matchElapsed) % 60;
        ImGui::Text("Duration: %02d:%02d", mins, secs);
        ImGui::Text("P1 units alive / lost: %d / %d", m_matchP1Alive, m_matchP1Lost);
        ImGui::Text("P2 units alive / lost: %d / %d", m_matchP2Alive, m_matchP2Lost);
        ImGui::Text("Oil spent: %.0f", m_matchOilSpent);

        ImGui::Spacing();

        if (ImGui::Button("Reset Match", ImVec2(-1, 36))) {
            m_resetRequested = true;
            m_victory = false;
            m_defeat  = false;
            m_gameStarted = false;
        }

        ImGui::End();
        ImGui::PopStyleColor();
    }

    // ===========================
    // START OVERLAY — pre-match "Press Start" screen
    // ===========================
    void GuiManager::RenderStartOverlay() {
        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImVec2 size(360, 160);
        ImGui::SetNextWindowPos(
            ImVec2(vp->WorkPos.x + (vp->WorkSize.x - size.x) * 0.5f,
                   vp->WorkPos.y + (vp->WorkSize.y - size.y) * 0.5f),
            ImGuiCond_Always);
        ImGui::SetNextWindowSize(size, ImGuiCond_Always);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.06f, 0.10f, 0.98f));
        ImGui::Begin("##StartOverlay", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

        ImGui::SetWindowFontScale(2.0f);
        const char* title = "READY";
        ImVec2 ts = ImGui::CalcTextSize(title);
        ImGui::SetCursorPosX((size.x - ts.x) * 0.5f);
        ImGui::TextColored(ImVec4(0.9f, 0.85f, 0.4f, 1.0f), "%s", title);
        ImGui::SetWindowFontScale(1.0f);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Start Game", ImVec2(-1, 36))) {
            m_gameStarted = true;
        }

        ImGui::End();
        ImGui::PopStyleColor();
    }

    // ===========================
    // FLOATING DAMAGE NUMBERS
    // ===========================
    void GuiManager::PushFloatingNumber(const glm::vec3& worldPos, int amount, bool kill) {
        // Cap the queue so a long firefight can't grow it unbounded.
        if (m_floatingNumbers.size() >= 64) m_floatingNumbers.erase(m_floatingNumbers.begin());
        m_floatingNumbers.push_back({ worldPos, static_cast<float>(amount), 0.0f, kill });
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

        // Floating damage numbers — drift upward, fade, drop after ~1s.
        const float kDamageNumberLifetime = 1.0f;
        for (size_t i = 0; i < m_floatingNumbers.size();) {
            FloatingNumber& fn = m_floatingNumbers[i];
            fn.age += m_deltaTime;
            if (fn.age >= kDamageNumberLifetime) {
                m_floatingNumbers.erase(m_floatingNumbers.begin() + i);
                continue;
            }
            glm::vec3 wp = fn.worldPos + glm::vec3(0.0f, fn.age * 25.0f + 4.0f, 0.0f);
            glm::vec4 clip = vpMat * glm::vec4(wp, 1.0f);
            if (clip.w <= 0.0001f) { ++i; continue; }
            glm::vec3 ndc = glm::vec3(clip) / clip.w;
            if (ndc.x < -1.2f || ndc.x > 1.2f || ndc.y < -1.2f || ndc.y > 1.2f) { ++i; continue; }
            const float sx = vpPos.x + (ndc.x * 0.5f + 0.5f) * vpSize.x;
            const float sy = vpPos.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * vpSize.y;
            const float t = fn.age / kDamageNumberLifetime;
            const int alpha = (int)(255 * (1.0f - t));
            const ImU32 col = fn.kill ? IM_COL32(255,  60,  60, alpha)   // red on kill
                                      : IM_COL32(255, 165,  40, alpha);  // orange on hit
            char buf[16];
            snprintf(buf, sizeof(buf), "-%d", (int)fn.amount);
            // Slightly larger for kill hits so they read at a glance.
            const float scale = fn.kill ? 1.5f : 1.2f;
            const float fontSize = ImGui::GetFontSize() * scale;
            ImFont* font = ImGui::GetFont();
            dl->AddText(font, fontSize, ImVec2(sx + 1, sy + 1),
                        IM_COL32(0, 0, 0, alpha), buf);
            dl->AddText(font, fontSize, ImVec2(sx, sy), col, buf);
            ++i;
        }
    }

} // namespace gps