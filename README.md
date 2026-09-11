# BM Planarity

C++20 implementacija Boyer--Myrvold algoritma za testiranje planarnosti grafa.

Implementacija:

- donosi odluku da li je graf planaran;
- za planarne grafove konstruiše kombinatorno planarno ulaganje;
- za neplanarne grafove izdvaja Kuratowskijev certifikat tipa `K5` ili `K3,3`;
- sadrži jedinične, regresijske i performansne testove;
- uključuje opcionalnu web demonstracijsku aplikaciju.


## Zahtjevi

Za osnovni build potrebni su:

- C++20 kompajler;
- CMake 3.20 ili noviji;
- Ninja, opciono.

Za regresijske testove potreban je i Python.

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
