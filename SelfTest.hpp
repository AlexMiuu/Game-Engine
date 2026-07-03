//
// SelfTest.hpp
// Teste functionale deterministe pentru capitolul 6 (tabelul 6.1).
//
// Verifica componentele cu comportament bine definit, pentru care rezultatul
// asteptat poate fi formulat precis: conversia hexagonala, cheltuirea atomica a
// resurselor, achizitia de tinte cu filtru de factiune, detectia coliziunilor,
// stergerea amanata la moartea unei unitati si constructia razei ortografice la
// marginile ecranului. Fiecare test isi construieste propriile obiecte de proba,
// deci rularea nu perturba starea jocului (exceptie: ResourceManager, singleton,
// caz in care valorile sunt salvate si restaurate).
//
// Testarea precisa a selectiei pe mesh ramane interactiva (are nevoie de modele
// incarcate prin OpenGL); aici este acoperita partea de matematica a razei, care
// este exact ce a fost corectat (raza cu origine variabila, directie constanta).
//

#ifndef SELF_TEST_HPP
#define SELF_TEST_HPP

#include <string>
#include <vector>

namespace gps {

    struct TestCase {
        std::string name;
        bool        passed = false;
        std::string detail;   // observatie scurta (ce a fost verificat / de ce a picat)
    };

    struct TestReport {
        std::vector<TestCase> cases;

        int  Passed() const;
        int  Total()  const { return static_cast<int>(cases.size()); }
        bool AllPassed() const { return Passed() == Total(); }

        std::string ToString() const;
    };

    class SelfTest {
    public:
        // Ruleaza toate testele deterministe si intoarce raportul.
        static TestReport RunFunctionalTests();

        // Scrie raportul in benchmark/functional_tests.txt (si in directorul
        // curent daca subfolderul nu e scriibil).
        static void WriteReport(const TestReport& report);
    };

} // namespace gps

#endif // SELF_TEST_HPP
