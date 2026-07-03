//
// Benchmark.hpp
// Harness de masurare a performantei pentru capitolul 6 (Testare si validare).
//
// Genereaza incarcarea descrisa in tabelele 6.2 si 6.3: spawneaza un numar
// crescator de nave in miscare continua, lasa aplicatia sa se stabilizeze
// (warmup), apoi mediaza rata de cadre, timpul pe cadru si consumul de memorie.
// Rezultatele sunt afisate in panoul de depanare si scrise intr-un fisier CSV
// pentru a completa tabelele si figurile din lucrare.
//
// Harness-ul NU detine niciun sistem: primeste pointeri imprumutati la scena,
// la managerul de scena si la managerul de tile-uri, exact ca celelalte sisteme
// din motor. Rularea se conduce cadru cu cadru prin Update(deltaTime), apelat o
// singura data pe cadru din bucla principala cu delta real.
//

#ifndef BENCHMARK_HPP
#define BENCHMARK_HPP

#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace gps {

    class Scene;
    class SceneManager;
    class TileManager;
    class SelectionSystem;
    class SceneObject;

    // Un singur punct masurat: o linie in tabelul 6.2 (sweep) sau 6.3 (stres).
    struct BenchmarkSample {
        int   unitCount  = 0;
        float avgFps     = 0.0f;
        float avgFrameMs = 0.0f;
        float minFps     = 0.0f;
        float maxFps     = 0.0f;
        float memoryMB   = 0.0f;
    };

    class BenchmarkHarness {
    public:
        // Faza curenta a masuratorii pentru un nivel de incarcare.
        enum class Phase { Idle, Preparing, Warmup, Measuring, Finished };
        // Ce tip de rulare este in desfasurare.
        enum class Mode { None, Sweep, Stress };

        BenchmarkHarness() = default;
        ~BenchmarkHarness() = default;

        BenchmarkHarness(const BenchmarkHarness&) = delete;
        BenchmarkHarness& operator=(const BenchmarkHarness&) = delete;

        // selectionSystem este optional; e folosit doar pentru a scoate din
        // selectie unitatile de benchmark cand sunt eliminate.
        void Initialize(Scene* scene, SceneManager* sceneManager,
                        TileManager* tileManager, SelectionSystem* selectionSystem = nullptr);

        // Tabelul 6.2: masoara o lista fixa de niveluri de incarcare.
        void StartSweep(const std::vector<int>& counts = { 100, 500, 1000, 2000, 5000 });
        // Tabelul 6.3: creste treptat pana cand rata medie coboara sub prag sau
        // se atinge plafonul de unitati.
        void StartStress(int startCount = 200, int step = 200,
                         float fpsThreshold = 30.0f, int maxUnits = 10000);
        // Opreste rularea si sterge unitatile de benchmark.
        void Cancel();

        // Se apeleaza o data pe cadru cu delta real (secunde).
        void Update(float deltaTime);

        // ---- Interogari pentru interfata ----
        bool  IsRunning() const {
            return m_mode != Mode::None && m_phase != Phase::Finished && m_phase != Phase::Idle;
        }
        Phase GetPhase() const { return m_phase; }
        Mode  GetMode()  const { return m_mode; }

        int   CurrentTargetCount()   const { return m_targetCount; }
        int   ActiveBenchmarkUnits() const { return static_cast<int>(m_unitIDs.size()); }
        float LiveAvgFps() const;
        float ProgressFraction() const;

        const std::vector<BenchmarkSample>& Results() const { return m_results; }
        const std::string& StatusText()  const { return m_status; }
        const std::string& LastCsvPath() const { return m_lastCsvPath; }

        // Rezultatul testului de stres (tabelul 6.3). -1 = neatins / nedeterminat.
        int StressFpsBelow30Count() const { return m_stressBelow30; }
        int StressMaxTested()       const { return m_stressMaxTested; }

        // Configurare
        void SetDisableVSync(bool v) { m_disableVSyncDuringRun = v; }
        bool GetDisableVSync() const { return m_disableVSyncDuringRun; }

        // Sterge toate navele spawnate de benchmark.
        void ClearBenchmarkUnits();

    private:
        void  BeginLevel(int count);
        void  EnsureUnitCount(int count);
        void  SpawnOneUnit();
        void  IssueWander(SceneObject* obj);
        void  ReissueWander(float deltaTime);
        void  RecordCurrentLevel();
        void  FinishRun();
        void  WriteCsv();
        void  ApplyVSync(bool enabled);
        float RandRange(float lo, float hi);

        static float QueryProcessMemoryMB();

        Scene*           m_scene           = nullptr;
        SceneManager*    m_sceneManager    = nullptr;
        TileManager*     m_tileManager     = nullptr;
        SelectionSystem* m_selectionSystem = nullptr;

        Mode  m_mode  = Mode::None;
        Phase m_phase = Phase::Idle;

        // Sweep (tabelul 6.2)
        std::vector<int> m_sweepCounts;
        int m_sweepIndex = 0;

        // Stres (tabelul 6.3)
        int   m_stressStep      = 200;
        float m_stressThreshold = 30.0f;
        int   m_stressMaxUnits  = 10000;
        int   m_stressBelow30   = -1;
        int   m_stressMaxTested = 0;

        int m_targetCount = 0;

        // Ferestre de timp
        float m_warmupDuration  = 1.5f;  // secunde ignorate inainte de masurare
        float m_measureDuration = 4.0f;  // secunde de esantioane mediate
        float m_phaseElapsed    = 0.0f;

        // Acumulatori peste fereastra de masurare
        double m_accFrameTime = 0.0;
        int    m_accFrames    = 0;
        float  m_minFrameMs   = 0.0f;
        float  m_maxFrameMs   = 0.0f;

        std::vector<int>             m_unitIDs;
        std::vector<BenchmarkSample> m_results;

        bool m_disableVSyncDuringRun = true;
        bool m_vsyncModified         = false;

        std::string m_status      = "Idle";
        std::string m_lastCsvPath;

        glm::vec3 m_spawnMin = glm::vec3(0.0f);
        glm::vec3 m_spawnMax = glm::vec3(0.0f);

        unsigned m_rngState    = 2463534242u;
        float    m_reissueTimer = 0.0f;
    };

} // namespace gps

#endif // BENCHMARK_HPP
