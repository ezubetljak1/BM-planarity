# BM Planarity

C++ projekat za implementaciju Boyer-Myrvold algoritma za testiranje planarnosti grafa, konstrukciju planar embeddinga i izdvajanje Kuratowski certifikata za neplanarne grafove.

## Struktura projekta

```text
BM-planarity/
  include/bm/     Header fajlovi biblioteke
  src/            Implementacija biblioteke
  tests/          Testovi
  tools/          Pomoćni CLI alati
  examples/       Primjeri ulaznih grafova
  docs/           Dokumentacija i implementacijski plan
  CMakeLists.txt  CMake konfiguracija
```

## Zahtjevi

Potrebno je imati instalirano:

- C++ kompajler sa podrškom za C++20
- CMake 3.20 ili noviji
- Ninja build system, opcionalno

Na Windowsu se može koristiti Visual Studio Build Tools / MSVC.

## Build i pokretanje testova

### Opcija 1: Build sa Ninja generatorom

Iz root foldera projekta pokrenuti:

```powershell
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

Test executable se može pokrenuti i direktno:

```powershell
.\build\bm_planarity_tests.exe
```

### Opcija 2: Build bez Ninja generatora

Ako Ninja nije instaliran, može se koristiti default CMake generator:

```powershell
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Kod Visual Studio generatora, test executable se najčešće nalazi u `Debug` folderu:

```powershell
.\build\Debug\bm_planarity_tests.exe
```

## Čišćenje build foldera

Ako se promijeni generator ili CMake zapne zbog cache-a, obrisati `build` folder i ponovo konfigurirati projekat:

```powershell
Remove-Item -Recurse -Force build
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

## Dodavanje novih fajlova

Ako se doda novi `.cpp` fajl u `src/`, potrebno ga je dodati u `add_library` dio u `CMakeLists.txt`.

Primjer:

```cmake
add_library(bm_planarity
    src/Graph.cpp
    src/NewFile.cpp
)
```

Ako se doda novi test fajl u `tests/`, potrebno ga je dodati u `add_executable` dio:

```cmake
add_executable(bm_planarity_tests
    tests/TestMain.cpp
    tests/GraphTest.cpp
    tests/NewTest.cpp
)
```

Nakon izmjene `CMakeLists.txt`, dovoljno je ponovo pokrenuti:

```powershell
cmake --build build
```

CMake će automatski regenerisati build fajlove ako je potrebno.

## Trenutni način testiranja

Projekat koristi jednostavan vlastiti test framework definisan u:

```text
tests/TestSupport.hpp
```

Testovi se pišu pomoću makroa:

```cpp
BM_TEST(NazivTesta) {
    BM_ASSERT(uslov);
}
```

Svi registrovani testovi se pokreću kroz:

```text
tests/TestMain.cpp
```

## Primjer očekivanog izlaza testova

```text
[PASS] GraphCreatesVerticesAndEdges
[PASS] GraphStoresAdjacencyEdgeIds
[PASS] GraphRejectsInvalidVertex
[PASS] GraphRejectsSelfLoop

Passed: 4
Failed: 0
```
