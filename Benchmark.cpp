//
// Benchmark.cpp
// Implementarea harness-ului de masurare a performantei (capitolul 6).
//

#include "Benchmark.hpp"

#include "Scene.hpp"
#include "SceneObject.hpp"
#include "SceneManager.hpp"
#include "TileManager.hpp"
#include "SelectionSystem.hpp"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <ctime>

// Interogarea memoriei procesului si crearea directorului de iesire folosesc
// API-ul Windows. NOMINMAX opreste macrourile min/max care ar intra in conflict
// cu std::min / std::max si cu glm.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

namespace gps {

    // Ground plane pe care stau navele; acelasi Y ca placement-ul cu mouse-ul.
    static constexpr float kBenchGroundY = -60.0f;

    void BenchmarkHarness::Initialize(Scene* scene, SceneManager* sceneManager,
                                      TileManager* tileManager, SelectionSystem* selectionSystem) {
        m_scene           = scene;
        m_sceneManager    = sceneManager;
        m_tileManager     = tileManager;
        m_selectionSystem = selectionSystem;
        std::cout << "BenchmarkHarness initialized" << std::endl;
    }

    // xorshift32 pentru pozitii reproductibile intre rulari.
    float BenchmarkHarness::RandRange(float lo, float hi) {
        m_rngState ^= m_rngState << 13;
        m_rngState ^= m_rngState >> 17;
        m_rngState ^= m_rngState << 5;
        const float u = (m_rngState & 0xFFFFFF) / static_cast<float>(0xFFFFFF);
        return lo + u * (hi - lo);
    }

    float BenchmarkHarness::QueryProcessMemoryMB() {
        PROCESS_MEMORY_COUNTERS pmc{};
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
            return static_cast<float>(pmc.WorkingSetSize / (1024.0 * 1024.0));
        return 0.0f;
    }

    void BenchmarkHarness::ApplyVSync(bool enabled) {
        glfwSwapInterval(enabled ? 1 : 0);
    }

    // ---------------------------------------------------------------------
    // Pornirea rularilor
    // ---------------------------------------------------------------------

    void BenchmarkHarness::StartSweep(const std::vector<int>& counts) {
        if (!m_scene || !m_sceneManager) {
            m_status = "Benchmark: sisteme nelegate";
            return;
        }
        ClearBenchmarkUnits();
        m_results.clear();
        m_sweepCounts = counts;
        std::sort(m_sweepCounts.begin(), m_sweepCounts.end());
        m_sweepIndex = 0;
        m_mode = Mode::Sweep;

        if (m_disableVSyncDuringRun) { ApplyVSync(false); m_vsyncModified = true; }
        if (!m_sweepCounts.empty()) BeginLevel(m_sweepCounts[0]);
        else FinishRun();
    }

    void BenchmarkHarness::StartStress(int startCount, int step, float fpsThreshold, int maxUnits) {
        if (!m_scene || !m_sceneManager) {
            m_status = "Benchmark: sisteme nelegate";
            return;
        }
        ClearBenchmarkUnits();
        m_results.clear();
        m_stressStep      = std::max(1, step);
        m_stressThreshold = fpsThreshold;
        m_stressMaxUnits  = maxUnits;
        m_stressBelow30   = -1;
        m_stressMaxTested = 0;
        m_mode = Mode::Stress;

        if (m_disableVSyncDuringRun) { ApplyVSync(false); m_vsyncModified = true; }
        BeginLevel(std::max(1, startCount));
    }

    void BenchmarkHarness::Cancel() {
        ClearBenchmarkUnits();
        if (m_vsyncModified) { ApplyVSync(true); m_vsyncModified = false; }
        m_mode  = Mode::None;
        m_phase = Phase::Idle;
        m_status = "Benchmark anulat";
    }

    void BenchmarkHarness::BeginLevel(int count) {
        m_targetCount = count;
        m_phase = Phase::Preparing;

        std::ostringstream os;
        os << "Se pregatesc " << count << " unitati...";
        m_status = os.str();

        EnsureUnitCount(count);

        // Reset ferestre / acumulatori si trecem in warmup.
        m_phaseElapsed = 0.0f;
        m_accFrameTime = 0.0;
        m_accFrames    = 0;
        m_minFrameMs   = 1e9f;
        m_maxFrameMs   = 0.0f;
        m_phase = Phase::Warmup;
    }

    // ---------------------------------------------------------------------
    // Spawn / miscare
    // ---------------------------------------------------------------------

    // Streambuf care arunca tot ce primeste (fara alocare, fara I/O).
    struct NullBuffer : std::streambuf {
        int overflow(int c) override { return c; } // pretinde ca a scris
    };

    // Suprima zgomotul de log al spawn-ului in masa (SpawnObject scrie o linie
    // per obiect). RAII: redirecteaza cout catre un buffer nul si il restaureaza
    // la iesirea din domeniu, curatand orice stare de eroare ramasa.
    struct CoutSilencer {
        std::streambuf* saved;
        NullBuffer      nullBuf;
        CoutSilencer()  : saved(std::cout.rdbuf(&nullBuf)) {}
        ~CoutSilencer() { std::cout.rdbuf(saved); std::cout.clear(); }
    };

    void BenchmarkHarness::EnsureUnitCount(int count) {
        // Recalculeaza limitele jucabile de fiecare data (harta se poate regenera).
        if (m_tileManager) {
            glm::vec3 mn, mx;
            if (m_tileManager->GetPlayableBounds(mn, mx)) { m_spawnMin = mn; m_spawnMax = mx; }
        }
        if (m_spawnMin == m_spawnMax) {
            // Fallback rezonabil daca limitele nu sunt disponibile.
            m_spawnMin = glm::vec3(-800.0f, kBenchGroundY, -800.0f);
            m_spawnMax = glm::vec3( 800.0f, kBenchGroundY,  800.0f);
        }

        CoutSilencer silence;
        while (static_cast<int>(m_unitIDs.size()) < count) SpawnOneUnit();
        while (static_cast<int>(m_unitIDs.size()) > count) {
            const int id = m_unitIDs.back();
            m_unitIDs.pop_back();
            if (m_selectionSystem) m_selectionSystem->RemoveFromSelection(id);
            if (m_scene) m_scene->DestroyObject(id);
        }
    }

    void BenchmarkHarness::SpawnOneUnit() {
        if (!m_sceneManager || !m_scene) return;

        glm::vec3 pos(RandRange(m_spawnMin.x, m_spawnMax.x),
                      kBenchGroundY,
                      RandRange(m_spawnMin.z, m_spawnMax.z));

        static int s_counter = 0;
        SceneObject* ship = m_sceneManager->SpawnObject(
            "BenchShip_" + std::to_string(s_counter++),
            "ship", "ship", pos, glm::vec3(6.5f));
        if (!ship) return;

        // Neutralizam lupta si ii dam HP urias: unitatile de benchmark nu trebuie
        // sa moara sau sa traga in timpul masurarii, ca N sa ramana constant
        // indiferent de starea meciului. Raman mobile si ciocnibile (raza setata
        // de SpawnObject), deci exercita bucla de miscare + coliziunea.
        ship->unitStats.isCombatUnit = false;
        ship->unitStats.faction      = 0;
        ship->unitStats.maxHealth    = 1000000;
        ship->unitStats.health       = 1000000;
        ship->patrolData.isPatrolling = false;

        IssueWander(ship);
        m_unitIDs.push_back(ship->GetID());
    }

    // Da unei nave o destinatie aleatoare in limitele jucabile. Bucla principala
    // integreaza miscarea si aplica coliziunea; ReissueWander reinnoieste ordinul
    // cand nava se opreste, ca incarcarea sa fie sustinuta.
    void BenchmarkHarness::IssueWander(SceneObject* obj) {
        if (!obj) return;
        glm::vec3 start = obj->GetTransform().GetPosition();
        glm::vec3 dest(RandRange(m_spawnMin.x, m_spawnMax.x),
                       kBenchGroundY,
                       RandRange(m_spawnMin.z, m_spawnMax.z));
        glm::vec3 dir = dest - start;
        dir.y = 0.0f;
        if (glm::length(dir) > 0.0001f) obj->movement.moveDirection = glm::normalize(dir);
        obj->movement.isMoving      = true;
        obj->movement.moveStartPos  = start;
        obj->movement.moveEndPos    = dest;
        obj->movement.moveStartTime = static_cast<float>(glfwGetTime());
        obj->movement.moveDuration  = 3.0f;
    }

    void BenchmarkHarness::ReissueWander(float deltaTime) {
        // Throttled: nu are rost sa scanam toate unitatile in fiecare cadru.
        m_reissueTimer += deltaTime;
        if (m_reissueTimer < 0.3f) return;
        m_reissueTimer = 0.0f;
        if (!m_scene) return;
        for (int id : m_unitIDs) {
            SceneObject* obj = m_scene->GetObjectByID(id);
            if (obj && !obj->movement.isMoving) IssueWander(obj);
        }
    }

    // ---------------------------------------------------------------------
    // Bucla de masurare
    // ---------------------------------------------------------------------

    void BenchmarkHarness::Update(float deltaTime) {
        if (m_mode == Mode::None) return;
        if (m_phase == Phase::Idle || m_phase == Phase::Finished) return;

        ReissueWander(deltaTime);

        if (m_phase == Phase::Warmup) {
            m_phaseElapsed += deltaTime;
            std::ostringstream os;
            os << "Stabilizare la " << m_targetCount << " unitati ("
               << static_cast<int>(m_phaseElapsed * 100.0f / m_warmupDuration) << "%)";
            m_status = os.str();
            if (m_phaseElapsed >= m_warmupDuration) {
                m_phaseElapsed = 0.0f;
                m_accFrameTime = 0.0;
                m_accFrames    = 0;
                m_minFrameMs   = 1e9f;
                m_maxFrameMs   = 0.0f;
                m_phase = Phase::Measuring;
            }
            return;
        }

        if (m_phase == Phase::Measuring) {
            m_phaseElapsed += deltaTime;
            if (deltaTime > 0.0f) {
                const float ms = deltaTime * 1000.0f;
                m_accFrameTime += deltaTime;
                m_accFrames    += 1;
                m_minFrameMs = std::min(m_minFrameMs, ms);
                m_maxFrameMs = std::max(m_maxFrameMs, ms);
            }
            std::ostringstream os;
            os << "Masurare la " << m_targetCount << " unitati ("
               << static_cast<int>(m_phaseElapsed * 100.0f / m_measureDuration) << "%)";
            m_status = os.str();

            if (m_phaseElapsed >= m_measureDuration) {
                RecordCurrentLevel();
            }
        }
    }

    void BenchmarkHarness::RecordCurrentLevel() {
        BenchmarkSample s;
        s.unitCount = m_targetCount;
        if (m_accFrames > 0 && m_accFrameTime > 0.0) {
            s.avgFrameMs = static_cast<float>(m_accFrameTime / m_accFrames * 1000.0);
            s.avgFps     = static_cast<float>(m_accFrames / m_accFrameTime);
        }
        s.minFps   = (m_maxFrameMs > 0.0f) ? (1000.0f / m_maxFrameMs) : 0.0f; // cadru cel mai lent
        s.maxFps   = (m_minFrameMs > 0.0f && m_minFrameMs < 1e8f) ? (1000.0f / m_minFrameMs) : 0.0f;
        s.memoryMB = QueryProcessMemoryMB();
        m_results.push_back(s);

        if (m_mode == Mode::Sweep) {
            ++m_sweepIndex;
            if (m_sweepIndex < static_cast<int>(m_sweepCounts.size()))
                BeginLevel(m_sweepCounts[m_sweepIndex]);
            else
                FinishRun();
        } else if (m_mode == Mode::Stress) {
            m_stressMaxTested = m_targetCount;
            if (s.avgFps < m_stressThreshold) {
                m_stressBelow30 = m_targetCount;   // primul nivel sub prag
                FinishRun();
            } else if (m_targetCount >= m_stressMaxUnits) {
                FinishRun();                       // plafon atins, inca peste prag
            } else {
                BeginLevel(m_targetCount + m_stressStep);
            }
        }
    }

    void BenchmarkHarness::FinishRun() {
        WriteCsv();
        if (m_vsyncModified) { ApplyVSync(true); m_vsyncModified = false; }
        m_phase = Phase::Finished;

        std::ostringstream os;
        if (m_mode == Mode::Stress) {
            if (m_stressBelow30 > 0)
                os << "Stres terminat: sub " << static_cast<int>(m_stressThreshold)
                   << " FPS la " << m_stressBelow30 << " unitati.";
            else
                os << "Stres terminat: inca peste prag la " << m_stressMaxTested << " unitati.";
        } else {
            os << "Sweep terminat: " << m_results.size() << " niveluri masurate.";
        }
        if (!m_lastCsvPath.empty()) os << " Salvat in " << m_lastCsvPath;
        m_status = os.str();
    }

    void BenchmarkHarness::WriteCsv() {
        if (m_results.empty()) return;

        CreateDirectoryA("benchmark", nullptr);
        const char* stem = (m_mode == Mode::Stress) ? "benchmark/stress_test.csv"
                                                     : "benchmark/perf_sweep.csv";
        std::ofstream f(stem);
        if (!f) { // fallback in directorul curent daca subfolderul nu e scriibil
            stem = (m_mode == Mode::Stress) ? "stress_test.csv" : "perf_sweep.csv";
            f.open(stem);
            if (!f) return;
        }

        std::time_t now = std::time(nullptr);
        std::tm tmBuf{};
        localtime_s(&tmBuf, &now);
        char tbuf[64] = {0};
        std::strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", &tmBuf);

        f << "# Generat de BenchmarkHarness la " << tbuf << "\n";
        f << "units,avg_fps,avg_frame_ms,min_fps,max_fps,memory_mb\n";
        f << std::fixed;
        f.precision(2);
        for (const BenchmarkSample& s : m_results) {
            f << s.unitCount << ',' << s.avgFps << ',' << s.avgFrameMs << ','
              << s.minFps << ',' << s.maxFps << ',' << s.memoryMB << '\n';
        }
        m_lastCsvPath = stem;
    }

    // ---------------------------------------------------------------------
    // Curatare + interogari live
    // ---------------------------------------------------------------------

    void BenchmarkHarness::ClearBenchmarkUnits() {
        if (m_scene) {
            for (int id : m_unitIDs) {
                if (m_selectionSystem) m_selectionSystem->RemoveFromSelection(id);
                m_scene->DestroyObject(id);
            }
        }
        m_unitIDs.clear();
    }

    float BenchmarkHarness::LiveAvgFps() const {
        if (m_accFrames > 0 && m_accFrameTime > 0.0)
            return static_cast<float>(m_accFrames / m_accFrameTime);
        return 0.0f;
    }

    float BenchmarkHarness::ProgressFraction() const {
        const float within = (m_phase == Phase::Warmup)
            ? (m_warmupDuration > 0.0f ? m_phaseElapsed / m_warmupDuration * 0.3f : 0.0f)
            : (m_phase == Phase::Measuring)
                ? (0.3f + (m_measureDuration > 0.0f ? m_phaseElapsed / m_measureDuration * 0.7f : 0.0f))
                : (m_phase == Phase::Finished ? 1.0f : 0.0f);

        if (m_mode == Mode::Sweep && !m_sweepCounts.empty()) {
            const float per = 1.0f / static_cast<float>(m_sweepCounts.size());
            return std::min(1.0f, (m_sweepIndex + within) * per);
        }
        return std::min(1.0f, within);
    }

} // namespace gps
