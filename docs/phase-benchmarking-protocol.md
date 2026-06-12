# Phase-level benchmark protocol

This diagnostic campaign isolates the cost of the non-planar certifying path for long subdivisions of `K3,3` and `K5`.

The production method `BoyerMyrvoldPlanarity::run()` is unchanged. The optional `runProfiled()` entry point records timings while executing the same algorithmic path.

## Top-level phases

- input validation;
- DFS preprocessing;
- embedding-state initialization;
- decision core (`Walkup`, `Walkdown`, tree-bicomp creation and blocked-state detection);
- unembedded-backedge failure-snapshot creation where applicable;
- Kuratowski-isolation preparation;
- Kuratowski minor isolation and path marking;
- independent certificate verification;
- planar embedding recovery where applicable;
- residual unaccounted time, including destruction of temporary state and timing overhead.

## Isolation-preparation subphases

- copying the preserved failure state;
- normalizing lazy orientations on the copied state;
- initializing the extraction context;
- classifying the A-E minor case;
- residual preparation overhead.

The A-E classifier is further decomposed into:

- initial A/B classification;
- external-face marking;
- highest X-Y path search;
- Z-to-root path search;
- future-pertinent search below the X-Y path;
- residual classifier overhead.

## Build

```powershell
cmake -S . -B build-eval -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DBM_ENABLE_BENCHMARKS=ON

cmake --build build-eval
```

## Quick diagnostic

```powershell
py tools\run_phase_benchmark_campaign.py `
    --cli .\build-eval\bm_planarity_phase_benchmark.exe `
    --profile quick `
    --plot
```

## Full diagnostic

```powershell
py tools\run_phase_benchmark_campaign.py `
    --cli .\build-eval\bm_planarity_phase_benchmark.exe `
    --profile full `
    --repetitions 15 `
    --warmups 3 `
    --plot
```
