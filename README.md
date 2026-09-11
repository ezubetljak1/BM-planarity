# BM-Planarity

C++ implementacija Boyer–Myrvold algoritma za testiranje planarnosti grafa,
razvijena u skladu sa standardom C++20.

Implementacija:

- donosi odluku da li je graf planaran;
- za planarne grafove konstruiše kombinatorno planarno ulaganje;
- za neplanarne grafove izdvaja Kuratowskijev certifikat tipa K5 ili K3,3;
- sadrži jedinične, regresijske i performansne testove;
- uključuje opcionalnu web demonstracijsku aplikaciju.


## Zahtjevi

Za osnovni build potrebni su:

- C++20 kompajler;
- CMake 3.20 ili noviji;
- Ninja, opciono.

Za regresijske testove potreban je i Python.

Za pokretanje web demonstracijske aplikacije potrebni su Node.js i npm.

## Build i testovi

Iz root foldera projekta:

```powershell
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

Ako Ninja nije instaliran:

```powershell
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Korištenje biblioteke

Biblioteka se može uključiti u drugi CMake projekat pomoću `add_subdirectory`
i povezati sa izvršnim targetom preko `target_link_libraries`.

Primjer `CMakeLists.txt`:

```cmake
add_subdirectory(BM-planarity)

add_executable(my_app main.cpp)

target_link_libraries(my_app PRIVATE bm_planarity)
```

U korisničkom kodu mogu se uključiti javni headeri:

```cpp
#include <bm/Graph.hpp>
#include <bm/BoyerMyrvoldPlanarity.hpp>
#include <bm/PlanarityResult.hpp>
```

Primjer korištenja:

```cpp
#include <bm/Graph.hpp>
#include <bm/BoyerMyrvoldPlanarity.hpp>
#include <bm/PlanarityResult.hpp>

#include <iostream>

int main() {
    bm::Graph graph(4);

    graph.addEdge(0, 1);
    graph.addEdge(0, 2);
    graph.addEdge(0, 3);
    graph.addEdge(1, 2);
    graph.addEdge(1, 3);
    graph.addEdge(2, 3);

    bm::BoyerMyrvoldPlanarity algorithm;
    bm::PlanarityResult result = algorithm.run(graph);

    if (result.planar) {
        std::cout << "Graf je planaran.\n";

        const auto& embedding = result.embedding->clockwiseEdgesAroundVertex;
        std::cout << "Planarno ulaganje:\n";

        for (int vertex = 0; vertex < embedding.size(); ++vertex) {
            std::cout << "Vrh " << vertex << ": ";

            for (int edgeId : embedding[vertex]) {
                std::cout << edgeId << " ";
            }
            std::cout << "\n";
        }
    } else {
        std::cout << "Graf nije planaran.\n";
        const auto& certificate = result.certificate->edgeIds;

        std::cout << "Kuratowskijev certifikat (ID-evi grana): ";
        for (int edgeId : certificate) {
            std::cout << edgeId << " ";
        }

        std::cout << "\n";
    }
}
```

Za planaran graf rezultat sadrži kombinatorno planarno ulaganje, dok za
neplanaran graf sadrži Kuratowskijev certifikat.

## Python regresijski testovi

Instalacija zavisnosti:

```powershell
py -m pip install -r tools\requirements.txt
```

Primjer diferencijalnog testiranja odluke o planarnosti:

```powershell
py tools\differential_regression.py `
    --cli .\build\bm_planarity_decision_cli.exe `
    --profile full
```

Projekat dodatno sadrži regresijske provjere planarnog ulaganja i
Kuratowskijevih certifikata.

## Demonstracijska aplikacija

Backend:

```powershell
.\build\bm_planarity_api.exe
```

Frontend:

```powershell
cd web
npm install
npm run dev
```

Web aplikacija omogućava unos grafa, pokretanje testa planarnosti i
vizuelni prikaz rezultata.


## Napomena

Implementacija je razvijena kao praktični dio završnog rada o algoritmima
za testiranje planarnosti grafova, sa fokusom na Boyer--Myrvold algoritam.
