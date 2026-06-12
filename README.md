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

## Differential regression testovi

Pored ciljanih C++ unit testova, projekat sadrži i opcionalni differential regression test sistem.

Cilj ovih testova je provjera odluke o planarnosti nad većim brojem različitih grafova. Za svaki generisani graf porede se:

```text
rezultat vlastite C++ implementacije Boyer-Myrvold algoritma
rezultat nezavisne NetworkX implementacije testa planarnosti
```

Provjerava se isključivo odluka:

```text
PLANAR / NONPLANAR
```

Testovi obuhvataju i planarne i neplanarne simple grafove.

Za poređenje se koristi funkcija:

```python
networkx.check_planarity(graph, counterexample=False)
```

Opcija `counterexample=False` znači da se tokom differential testiranja ne izdvaja Kuratowski certifikat, nego se provjerava samo odluka o planarnosti.

### Struktura differential test alata

```text
tools/
  BmDecisionCli.cpp           Batch CLI wrapper oko C++ biblioteke
  differential_regression.py Python skripta za generisanje i poređenje grafova
  requirements.txt           Python zavisnosti
```

`BmDecisionCli.cpp` omogućava da Python skripta pošalje više grafova jednom pokrenutom C++ procesu. Time se izbjegava ponovno pokretanje executable fajla za svaki pojedinačni graf.

Python skripta koristi više grupa grafova:

```text
Graph Atlas grafovi
svi mali simple grafovi generisani iscrpnom pretragom
nasumično generisani G(n, p) grafovi
```

Ako skripta pronađe razliku između C++ implementacije i NetworkX rezultata, ispisuje konkretan graf i njegovu listu grana kako bi se slučaj mogao trajno dodati kao novi C++ regresijski test.

## Instalacija Python zavisnosti

Za pokretanje differential regression testova potrebno je imati instaliran Python.

Iz root foldera projekta pokrenuti:

```powershell
py -m pip install -r tools\requirements.txt
```

## Build sa uključenim differential testovima

Differential testovi su opcionalni i podrazumijevano nisu uključeni u standardni build.

Za uključivanje testova potrebno je ponovo konfigurirati projekat:

```powershell
cmake -S . -B build -G Ninja -DBM_ENABLE_DIFFERENTIAL_TESTS=ON
cmake --build build
```

Ako se koristi default CMake generator:

```powershell
cmake -S . -B build -DBM_ENABLE_DIFFERENTIAL_TESTS=ON
cmake --build build
```

## Pokretanje brzog differential testa

Brzi profil je namijenjen redovnoj provjeri tokom razvoja:

```powershell
py tools\differential_regression.py `
    --cli .\build\bm_planarity_decision_cli.exe `
    --profile quick
```

Kod Visual Studio generatora CLI executable se najčešće nalazi u `Debug` folderu:

```powershell
py tools\differential_regression.py `
    --cli .\build\Debug\bm_planarity_decision_cli.exe `
    --profile quick
```

Ako su differential testovi uključeni kroz CMake, brzi profil se može pokrenuti i kroz CTest:

```powershell
ctest --test-dir build --output-on-failure -L differential
```

## Pokretanje punog differential testa

Puni profil obuhvata znatno veći broj grafova i preporučuje se prije merge-a većih algoritamskih izmjena u `main` granu:

```powershell
py tools\differential_regression.py `
    --cli .\build\bm_planarity_decision_cli.exe `
    --profile full
```

Kod Visual Studio generatora:

```powershell
py tools\differential_regression.py `
    --cli .\build\Debug\bm_planarity_decision_cli.exe `
    --profile full
```

## Uloga pojedinačnih vrsta testova

Ciljani C++ testovi i differential regression testovi imaju različite uloge:

```text
C++ unit testovi
    provjeravaju pojedinačne strukture, operacije i poznate grafove
    precizno lociraju fazu algoritma u kojoj se pojavila greška

Differential regression testovi
    provjeravaju odluku planarnosti nad velikim brojem grafova
    otkrivaju neočekivane regresije i rubne slučajeve
    porede rezultat sa nezavisnom implementacijom
```

Preporučeni redoslijed provjere prije merge-a u `main` granu:

```powershell
cmake --build build
ctest --test-dir build --output-on-failure
py tools\differential_regression.py `
    --cli .\build\bm_planarity_decision_cli.exe `
    --profile full
```



## Validacija recovered planar embeddinga

Nakon uspješne odluke da je graf planaran, biblioteka više ne vraća placeholder nego stvarni
rotation system originalnih edge ID-jeva:

```cpp
result.embedding->clockwiseEdgesAroundVertex[vertex]
```

Svaki unutrašnji vektor sadrži ciklični redoslijed grana incidentnih na odgovarajući vertex.
Globalna refleksija embeddinga je dozvoljena, pa redoslijed ne mora biti identičan redoslijedu koji
vrati druga biblioteka; bitno je da opisuje validan planarni embedding.

C++ klasa:

```text
PlanarEmbeddingValidator
```

nezavisno provjerava:

```text
svaki originalni edge se pojavljuje tačno jednom oko oba endpointa
lokalni redoslijedi odgovaraju stepenima vertexa
obilazak lica zatvara cikluse
Eulerova relacija važi za svaku povezanu komponentu
```

### NetworkX embedding regression testovi

Pored differential poređenja odluke planarnosti, projekat sadrži i opcionalnu provjeru recovered
rotation systema kroz `networkx.PlanarEmbedding.check_structure()`.

Dodatni alati:

```text
tools/
  BmEmbeddingCli.cpp       Batch CLI koji ispisuje recovered rotation system
  embedding_regression.py Python skripta koja validira rotation system kroz NetworkX
```

Brzi profil:

```powershell
py tools\embedding_regression.py `
    --cli .\build\bm_planarity_embedding_cli.exe `
    --profile quick
```

Puni profil prije merge-a većih izmjena:

```powershell
py tools\embedding_regression.py `
    --cli .\build\bm_planarity_embedding_cli.exe `
    --profile full
```

Ako su opcionalni Python testovi uključeni kroz CMake:

```powershell
cmake -S . -B build -G Ninja -DBM_ENABLE_DIFFERENTIAL_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure -L differential
```

Tada CTest pokreće i provjeru odluke planarnosti i provjeru recovered embeddinga.


## Validacija Kuratowski certifikata

Prije implementacije automatskog izdvajanja Kuratowski certifikata dodan je nezavisni linearni verifier:

```text
KuratowskiCertificateVerifier
```

Verifier prima originalni graf i skup originalnih edge ID-jeva te provjerava da li odabrane grane
čine subdiviziju grafa `K5` ili `K3,3`. Provjera radi bez oslanjanja na Boyer-Myrvold decision core:

```text
provjeri validnost i jedinstvenost originalnih edge ID-jeva
izgradi podgraf certifikata
prepoznaj branch vertexe i degree-2 subdivision vertexe
potisni degree-2 puteve u kernel grane
provjeri da kernel odgovara K5 ili K3,3
```

Ukupna složenost verifiera je linearna u veličini ulaznog grafa i certifikata:

```text
O(n + m)
```

### NetworkX regresija verifiera

Dodatni alati:

```text
tools/
  BmKuratowskiVerifierCli.cpp       Batch CLI oko C++ verifiera
  kuratowski_verifier_regression.py Python skripta za široku provjeru verifiera
```

Python skripta generiše neplanarne grafove, poziva:

```python
networkx.check_planarity(graph, counterexample=True)
```

zatim mapira grane NetworkX kontraprimjera na originalne edge ID-jeve i šalje ih C++ verifieru.
Na taj način se verifier testira na većem skupu različitih `K5` i `K3,3` subdivizija.

Brzi profil:

```powershell
py tools\kuratowski_verifier_regression.py `
    --cli .\build\bm_kuratowski_verifier_cli.exe `
    --profile quick
```

Puni profil:

```powershell
py tools\kuratowski_verifier_regression.py `
    --cli .\build\bm_kuratowski_verifier_cli.exe `
    --profile full
```

Kod Visual Studio generatora executable se najčešće nalazi u `Debug` folderu:

```powershell
py tools\kuratowski_verifier_regression.py `
    --cli .\build\Debug\bm_kuratowski_verifier_cli.exe `
    --profile quick
```

Ako su opcionalni Python testovi uključeni kroz CMake, brzi verifier regression ulazi i u:

```powershell
ctest --test-dir build --output-on-failure -L differential
```

Napomena: ovaj sloj validira certifikate, ali još ne izdvaja certifikat automatski iz
Boyer-Myrvold failure stanja. Referentni A-E isolator predstavlja narednu fazu implementacije.

## Kuratowski certifikati za neplanarne grafove

Za neplanaran graf javni rezultat sadrži `KuratowskiCertificate` sa originalnim ID-jevima
garana koje čine subdiviziju `K5` ili `K3,3`. Ekstrakcija se pokreće direktno iz
sačuvanog Walkdown failure konteksta; ne pokreće ponovo test planarnosti i ne koristi
external-face shortcut linkove kao stvarne grane certifikata.

Certifikat se nezavisno provjerava u C++ sloju pomoću:

```text
KuratowskiCertificateVerifier
```

Verifier logički potiskuje degree-2 subdivision vertexe i zahtijeva da preostali kernel
bude tačno `K5` ili `K3,3`.

### NetworkX regresija automatski izdvojenih certifikata

Pored unit testova postoji i opcionalni regression alat:

```text
tools/BmCertificateCli.cpp
tools/kuratowski_extractor_regression.py
```

Za svaki generisani graf skripta:

```text
provjerava odluku PLANAR / NONPLANAR pomoću NetworkX-a
za svaki NONPLANAR rezultat učitava originalne edge ID-jeve iz C++ certifikata
nezavisno potiskuje degree-2 subdivision puteve
provjerava da je dobijeni kernel izomorfan sa K5 ili K3,3
```

Brzi profil:

```powershell
py tools\kuratowski_extractor_regression.py `
    --cli .\build\bm_kuratowski_certificate_cli.exe `
    --profile quick
```

Puni profil prije merge-a većih izmjena:

```powershell
py tools\kuratowski_extractor_regression.py `
    --cli .\build\bm_kuratowski_certificate_cli.exe `
    --profile full
```

Kod Visual Studio generatora executable se najčešće nalazi u `build\Debug\` folderu.

Ako je projekat konfigurisan sa:

```powershell
cmake -S . -B build -G Ninja -DBM_ENABLE_DIFFERENTIAL_TESTS=ON
```

brzi Kuratowski regression test pokreće se i kroz:

```powershell
ctest --test-dir build --output-on-failure -L kuratowski
```
