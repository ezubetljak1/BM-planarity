# Protokol za evaluaciju Boyer–Myrvold implementacije

## 1. Razdvajanje correctness i performance evaluacije

Evaluacija je podijeljena na dva odvojena sloja:

1. **Correctness kampanja** poredi odluke, recovered embeddinge i Kuratowski certifikate sa nezavisnim NetworkX oracleom.
2. **Performance kampanja** mjeri isključivo direktan poziv `BoyerMyrvoldPlanarity::run(graph)` u Release buildu. U izmjereno vrijeme nisu uključeni HTTP, JSON, OGDF layout ni React frontend.

## 2. Correctness kampanja

Postojeće regresijske skripte pokrivaju:

- sve grafove iz NetworkX Graph Atlas skupa;
- sve označene simple neusmjerene grafove do šest čvorova u `full` profilu;
- nasumično generisane grafove sa fiksiranim seedovima;
- provjeru odluke `PLANAR/NONPLANAR`;
- nezavisnu provjeru recovered rotation systema;
- nezavisnu provjeru izdvojenih Kuratowski certifikata;
- smoke test A–E klasifikacije Kuratowski minor slučajeva.

Za završni izvještaj koristi se orkestrator:

```powershell
py tools\run_validation_campaign.py `
    --build-dir build `
    --profile thesis
```

Profil `thesis` ponavlja random kampanju nad više seedova i sprema logove, metadata JSON i Markdown sažetak u `results/validation/`.

## 3. Performance kampanja

Benchmark executable generiše grafove u memoriji izvan izmjerenog intervala, obavlja warmup pozive, a zatim mjeri direktan poziv:

```cpp
BoyerMyrvoldPlanarity algorithm;
algorithm.run(graph);
```

Koristi se `std::chrono::steady_clock`. Svaki scenario se ponavlja više puta. U sirovi CSV spremaju se svako pojedinačno mjerenje, familija grafa, `n`, `m`, `n + m`, odluka, seed i indeks instance.

### Familije grafova

| Familija | Namjena |
|---|---|
| `path` | dubok DFS put; rijedak planaran slučaj |
| `cycle` | minimalni planarni ciklusi |
| `wheel` | planarni graf sa većim stepenom centralnog čvora |
| `grid` | planarni mrežasti grafovi |
| `stacked_triangulation` | maksimalno planarni grafovi sa `3n - 6` grana |
| `random_tree` | nasumična planarna stabla |
| `random_sparse` | nasumični simple grafovi blizu granice `3n - 6` |
| `subdivided_k33` | neplanarni grafovi sa dugim subdivision putevima |
| `subdivided_k5` | neplanarni grafovi sa dugim subdivision putevima |
| `complete` | gusti ulazi; provjera dense-input shortcut grane |

### Pokretanje

```powershell
cmake -S . -B build -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DBM_ENABLE_BENCHMARKS=ON

cmake --build build

py tools\run_benchmark_campaign.py `
    --cli .\build\bm_planarity_benchmark.exe `
    --profile full `
    --plot
```

Na Linuxu se iz putanje izostavlja `.exe`.

## 4. Statistička obrada

Za svaku familiju i veličinu ulaza izračunavaju se:

- medijana vremena;
- prvi i treći kvartil;
- minimum i maksimum;
- medijana normalizovanog vremena `t / (n + m)`;
- empirijski log-log nagib na većoj polovini ulaza.

Medijana i kvartili su prikladniji od jednog mjerenja ili samog prosjeka jer su otporniji na povremene smetnje operativnog sistema.

Nagib blizu `1` na grafiku `log(t)` u odnosu na `log(n + m)` empirijski je kompatibilan sa linearnim rastom. To nije zamjena za formalni dokaz složenosti algoritma.

## 5. Figure

`tools/plot_benchmarks.py` izvozi svaku figuru u tri formata:

- `PNG` sa 300 DPI za pregled i prezentaciju;
- `PDF` kao vektorski format za LaTeX rad;
- `SVG` kao vektorski format za naknadno uređivanje.

Stil je namjerno sličan MATLAB primjeru: fiksirana širina figure u centimetrima, kontrolisan odnos visine i širine, uklonjene gornja i desna osa, čitljive oznake i diskretna mreža.

## 6. Reproducibilnost

Uz rezultate se čuvaju:

- git commit;
- status radnog stabla;
- timestamp;
- operativni sistem i arhitektura;
- Python verzija;
- kompletna izvršena komanda;
- seedovi;
- sirovi CSV rezultati.

Mjerenja za rad treba pokretati na istoj mašini, uz zatvorene nepotrebne aplikacije, priključen punjač i stabilan power plan. Benchmark treba graditi isključivo u `Release` modu bez sanitizera i debug opcija.

## 7. Preporučeni skup izvještajnih artefakata

U završni rad je dovoljno uključiti:

1. tabelu correctness kampanje sa brojem provjerenih slučajeva i nulom pronađenih odstupanja;
2. grafik `runtime_vs_work_size`;
3. grafik `normalized_runtime`;
4. poseban grafik `dense_complete_runtime_vs_m`;
5. tabelu empirijskih nagiba po familijama;
6. kratak opis hardverskog i softverskog okruženja.
