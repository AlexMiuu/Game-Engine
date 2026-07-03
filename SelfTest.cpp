//
// SelfTest.cpp
// Implementarea testelor functionale deterministe (tabelul 6.1).
//

#include "SelfTest.hpp"

#include "Scene.hpp"
#include "SceneObject.hpp"
#include "CombatSystem.hpp"
#include "CollisionSystem.hpp"
#include "TileManager.hpp"
#include "ResourceManager.hpp"
#include "Camera.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <ctime>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace gps {

    int TestReport::Passed() const {
        int n = 0;
        for (const TestCase& c : cases) if (c.passed) ++n;
        return n;
    }

    std::string TestReport::ToString() const {
        std::ostringstream os;
        os << "Rezultat teste functionale: " << Passed() << " / " << Total() << " trecute\n";
        for (const TestCase& c : cases) {
            os << (c.passed ? "  [PASS] " : "  [FAIL] ") << c.name;
            if (!c.detail.empty()) os << "  ->  " << c.detail;
            os << "\n";
        }
        return os.str();
    }

    namespace {

        std::string F(float v) {
            std::ostringstream os;
            os.setf(std::ios::fixed);
            os.precision(3);
            os << v;
            return os.str();
        }

        bool Approx(float a, float b, float eps = 0.001f) { return std::fabs(a - b) < eps; }

        // --- Rand 1: conversia hexagonala ---------------------------------
        TestCase TestHexConversion() {
            TestCase tc;
            tc.name = "Conversia hexagonala (round-trip + rotunjire cubica la muchii)";
            TileManager tm(100.566f, glm::vec3(18.0f), -140);
            const float s = 100.566f;

            // (a) Round-trip exact pe centrele celulelor.
            bool roundTrip = true;
            int fgx = 0, fgz = 0;
            for (int gx = -8; gx <= 8 && roundTrip; ++gx)
                for (int gz = -8; gz <= 8 && roundTrip; ++gz) {
                    glm::vec3 w = tm.GridToWorld(gx, gz);
                    int rgx, rgz;
                    tm.WorldToGrid(w, rgx, rgz);
                    if (rgx != gx || rgz != gz) { roundTrip = false; fgx = gx; fgz = gz; }
                }

            // (b) Proprietatea de celula cea mai apropiata: rotunjirea cubica
            // garanteaza ca orice punct din lume, inclusiv langa muchii, este
            // atribuit hexului cu centrul cel mai apropiat. Verificam ca niciun
            // hex vecin nu este strict mai aproape decat cel atribuit.
            bool nearest = true;
            float worstSlack = 0.0f;
            for (int i = 0; i <= 48 && nearest; ++i)
                for (int j = 0; j <= 48 && nearest; ++j) {
                    const float fx = -6.0f + 12.0f * (i / 48.0f);
                    const float fz = -6.0f + 12.0f * (j / 48.0f);
                    glm::vec3 wp(fx * s, 0.0f, fz * s);
                    int cgx, cgz;
                    tm.WorldToGrid(wp, cgx, cgz);
                    glm::vec3 assigned = tm.GridToWorld(cgx, cgz);
                    const float dA = glm::distance(glm::vec2(wp.x, wp.z),
                                                   glm::vec2(assigned.x, assigned.z));
                    for (int dx = -2; dx <= 2 && nearest; ++dx)
                        for (int dz = -2; dz <= 2 && nearest; ++dz) {
                            if (dx == 0 && dz == 0) continue;
                            glm::vec3 c = tm.GridToWorld(cgx + dx, cgz + dz);
                            const float d = glm::distance(glm::vec2(wp.x, wp.z),
                                                          glm::vec2(c.x, c.z));
                            if (d + 0.01f < dA) { nearest = false; worstSlack = dA - d; }
                        }
                }

            tc.passed = roundTrip && nearest;
            if (!roundTrip) {
                std::ostringstream os;
                os << "round-trip esuat la celula (" << fgx << ", " << fgz << ")";
                tc.detail = os.str();
            } else if (!nearest) {
                tc.detail = "un vecin era mai aproape cu " + F(worstSlack) + " unitati (rotunjire gresita)";
            } else {
                tc.detail = "289 celule round-trip + 2401 puncte atribuite corect celui mai apropiat hex";
            }
            return tc;
        }

        // --- Rand 4: cheltuirea resurselor --------------------------------
        TestCase TestResourceSpend() {
            TestCase tc;
            tc.name = "Cheltuirea resurselor (verificare integrala inainte de deducere)";
            ResourceManager& rm = ResourceManager::Instance();

            // Salveaza si restaureaza: ResourceManager e singleton, partajat cu jocul.
            const float savedOil  = rm.Get("Oil");
            const float savedFish = rm.Get("Fish");

            rm.Set("Oil", 100.0f);
            rm.Set("Fish", 10.0f);

            // Cost pe doua resurse, una insuficienta -> respins, nimic dedus.
            const bool rejected  = !rm.Spend({ {"Oil", 50.0f}, {"Fish", 20.0f} });
            const bool untouched = Approx(rm.Get("Oil"), 100.0f) && Approx(rm.Get("Fish"), 10.0f);

            // Cost acoperibil -> acceptat, dedus integral.
            const bool accepted = rm.Spend({ {"Oil", 50.0f}, {"Fish", 5.0f} });
            const bool deducted = Approx(rm.Get("Oil"), 50.0f) && Approx(rm.Get("Fish"), 5.0f);

            rm.Set("Oil", savedOil);
            rm.Set("Fish", savedFish);

            tc.passed = rejected && untouched && accepted && deducted;
            if (!rejected)        tc.detail = "achizitia insuficienta nu a fost respinsa";
            else if (!untouched)  tc.detail = "s-a dedus partial dintr-o achizitie respinsa (economie corupta)";
            else if (!accepted)   tc.detail = "achizitia acoperibila a fost respinsa gresit";
            else if (!deducted)   tc.detail = "deducerea achizitiei valide a fost incorecta";
            else                  tc.detail = "respins fara deducere; acceptat cu deducere integrala";
            return tc;
        }

        // --- Rand 7: achizitia de tinte -----------------------------------
        TestCase TestTargetAcquisition() {
            TestCase tc;
            tc.name = "Achizitia de tinte (cel mai apropiat inamic, filtrat pe factiuni)";
            Scene scene("SelfTestCombat");

            auto mk = [&](const char* n, int faction, glm::vec3 pos, bool combat) {
                SceneObject* o = scene.CreateObject(n);
                o->unitStats.faction     = faction;
                o->unitStats.isAlive     = true;
                o->unitStats.isCombatUnit = combat;
                o->GetTransform().SetPosition(pos);
                o->UpdateWorldBounds();
                return o;
            };

            SceneObject* atk = mk("Attacker", 1, glm::vec3(0.0f), true);
            atk->unitStats.attackRange       = 1000.0f;
            atk->unitStats.attackMode        = AttackMode::Melee;
            atk->unitStats.baseAttackCooldown = 1.0f;
            atk->unitStats.attackCooldown    = 0.0f;
            atk->unitStats.stance            = CombatStance::Neutral;
            atk->unitStats.isMovable         = true;

            SceneObject* ally      = mk("Ally",      1, glm::vec3(10.0f, 0.0f, 0.0f), false);
            SceneObject* enemyNear = mk("EnemyNear", 2, glm::vec3(20.0f, 0.0f, 0.0f), false);
            SceneObject* enemyFar  = mk("EnemyFar",  2, glm::vec3(60.0f, 0.0f, 0.0f), false);
            (void)enemyFar;

            CombatSystem cs;
            cs.Initialize(&scene, nullptr);   // calea melee nu foloseste SceneManager
            cs.Update(0.016f);

            const bool pickedNearEnemy = (atk->unitStats.targetID == enemyNear->GetID());
            const bool ignoredAlly     = (atk->unitStats.targetID != ally->GetID());

            tc.passed = pickedNearEnemy && ignoredAlly;
            if (!ignoredAlly)          tc.detail = "a tintit un aliat din aceeasi factiune";
            else if (!pickedNearEnemy) tc.detail = "nu a ales inamicul cel mai apropiat din raza";
            else                       tc.detail = "aliatul apropiat ignorat, atacat inamicul cel mai apropiat";
            return tc;
        }

        // --- Rand 5: coliziuni --------------------------------------------
        TestCase TestCollision() {
            TestCase tc;
            tc.name = "Coliziuni (detectie sfera-sfera, raza nula ignorata)";
            Scene scene("SelfTestCollision");
            CollisionSystem cs;
            cs.Initialize(&scene);

            SceneObject* a = scene.CreateObject("A");
            a->SetCollisionRadius(25.0f);
            a->GetTransform().SetPosition(glm::vec3(0.0f));
            a->UpdateWorldBounds();

            SceneObject* b = scene.CreateObject("B");
            b->SetCollisionRadius(25.0f);
            b->GetTransform().SetPosition(glm::vec3(30.0f, 0.0f, 0.0f)); // dist 30 < 50
            b->UpdateWorldBounds();
            const bool overlaps = (cs.GetCollidingObject(a) == b);

            b->GetTransform().SetPosition(glm::vec3(100.0f, 0.0f, 0.0f)); // dist 100 > 50
            b->UpdateWorldBounds();
            const bool apart = (cs.GetCollidingObject(a) == nullptr);

            b->GetTransform().SetPosition(glm::vec3(10.0f, 0.0f, 0.0f));
            b->UpdateWorldBounds();
            b->SetCollisionRadius(0.0f);                                  // ne-ciocnibil
            const bool nonCollidable = (cs.GetCollidingObject(a) == nullptr);

            tc.passed = overlaps && apart && nonCollidable;
            if (!overlaps)            tc.detail = "suprapunere reala nedetectata";
            else if (!apart)          tc.detail = "coliziune raportata gresit intre sfere departate";
            else if (!nonCollidable)  tc.detail = "obiect cu raza 0 raportat drept coliziune";
            else                      tc.detail = "detectat suprapus, liber departat, ignorat raza nula";
            return tc;
        }

        // --- Rand 6: moartea in timpul actualizarii -----------------------
        TestCase TestDeferredDeath() {
            TestCase tc;
            tc.name = "Moartea in timpul actualizarii (marcare + curatare amanata)";
            Scene scene("SelfTestDeath");

            std::vector<int> ids;
            for (int i = 0; i < 10; ++i)
                ids.push_back(scene.CreateObject("U" + std::to_string(i))->GetID());

            // Simuleaza marcarea in timpul unei parcurgeri: NU stergem in bucla
            // (asta ar invalida iteratorul), ci colectam ID-urile de eliminat.
            std::vector<int> toKill;
            int idx = 0;
            for (const auto& p : scene.GetObjects())
                if ((idx++ % 2) == 0) toKill.push_back(p->GetID());

            // Curatare amanata, dupa incheierea parcurgerii.
            for (int id : toKill) scene.DestroyObject(id);

            const bool countOk = (scene.GetObjectCount() == static_cast<size_t>(10 - (int)toKill.size()));

            bool killedGone = true;
            for (int id : toKill)
                if (scene.GetObjectByID(id) != nullptr) killedGone = false;

            bool survivorsKept = true;
            for (int id : ids) {
                const bool wasKilled = std::find(toKill.begin(), toKill.end(), id) != toKill.end();
                if (!wasKilled && scene.GetObjectByID(id) == nullptr) survivorsKept = false;
            }

            tc.passed = countOk && killedGone && survivorsKept;
            if (!countOk)             tc.detail = "numar de obiecte inconsistent dupa stergere";
            else if (!killedGone)     tc.detail = "un obiect marcat mort a supravietuit";
            else if (!survivorsKept)  tc.detail = "un supravietuitor a fost sters din greseala";
            else                      tc.detail = "5 marcate, sterse amanat, 5 pastrate, parcurgere valida";
            return tc;
        }

        // --- Rand 2: raza de selectie la marginile ecranului --------------
        TestCase TestOrthoRayEdges() {
            TestCase tc;
            tc.name = "Selectia prin raza la marginile ecranului (raza ortografica)";

            const int   W = 1920, H = 1080;
            const float aspect    = static_cast<float>(W) / static_cast<float>(H);
            const float orthoSize = 150.0f;
            const float groundY   = -60.0f;

            Camera cam(glm::vec3(0.0f, 200.0f, -200.0f));
            glm::mat4 proj  = glm::ortho(-orthoSize * aspect, orthoSize * aspect,
                                         -orthoSize, orthoSize, -1000.0f, 1000.0f);
            glm::mat4 view  = cam.getViewMatrix();
            glm::mat4 invVP = glm::inverse(proj * view);
            glm::vec3 front = glm::normalize(cam.getCameraFrontDirection());

            // Centru, 4 colturi si 4 mijloace de muchie: exact zonele unde raza
            // ortografica cu origine fixa gresea inainte de corectare.
            const glm::vec2 pts[] = {
                { W * 0.5f, H * 0.5f },
                { 2.0f, 2.0f }, { W - 2.0f, 2.0f }, { 2.0f, H - 2.0f }, { W - 2.0f, H - 2.0f },
                { W * 0.5f, 2.0f }, { W * 0.5f, H - 2.0f }, { 2.0f, H * 0.5f }, { W - 2.0f, H * 0.5f }
            };

            bool  ok    = true;
            float worst = 0.0f;
            if (std::fabs(front.y) < 1e-6f) {
                tc.passed = false;
                tc.detail = "directia camerei este orizontala, planul nu poate fi intersectat";
                return tc;
            }
            for (const glm::vec2& p : pts) {
                const float xN = 2.0f * p.x / W - 1.0f;
                const float yN = 1.0f - 2.0f * p.y / H;
                glm::vec4 npNDC(xN, yN, -1.0f, 1.0f);
                glm::vec4 npW = invVP * npNDC;
                if (npW.w != 0.0f) npW /= npW.w;

                // Raza: origine variabila (punctul deproiectat), directie constanta.
                glm::vec3 origin(npW);
                const float t = (groundY - origin.y) / front.y;
                glm::vec3 world = origin + t * front;

                // Reproiectare in spatiul ecranului.
                glm::vec4 clip = proj * view * glm::vec4(world, 1.0f);
                if (clip.w != 0.0f) clip /= clip.w;
                const float sx = (clip.x + 1.0f) * 0.5f * W;
                const float sy = (1.0f - clip.y) * 0.5f * H;

                const float err = glm::distance(glm::vec2(sx, sy), p);
                worst = std::max(worst, err);
                if (err > 1.0f) ok = false;
            }

            tc.passed = ok;
            tc.detail = "eroare maxima de reproiectare " + F(worst) + " px (prag 1 px), colturi incluse";
            return tc;
        }

    } // namespace

    TestReport SelfTest::RunFunctionalTests() {
        TestReport report;
        report.cases.push_back(TestHexConversion());
        report.cases.push_back(TestOrthoRayEdges());
        report.cases.push_back(TestResourceSpend());
        report.cases.push_back(TestCollision());
        report.cases.push_back(TestDeferredDeath());
        report.cases.push_back(TestTargetAcquisition());
        return report;
    }

    void SelfTest::WriteReport(const TestReport& report) {
        CreateDirectoryA("benchmark", nullptr);
        const char* path = "benchmark/functional_tests.txt";
        std::ofstream f(path);
        if (!f) { path = "functional_tests.txt"; f.open(path); if (!f) return; }

        std::time_t now = std::time(nullptr);
        std::tm tmBuf{};
        localtime_s(&tmBuf, &now);
        char tbuf[64] = { 0 };
        std::strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", &tmBuf);

        f << "Raport teste functionale (tabelul 6.1) - " << tbuf << "\n";
        f << "==================================================\n";
        f << report.ToString();
    }

} // namespace gps
