# Implementation Plan

## Goal

Implement a certifying planarity testing tool based on the Boyer–Myrvold edge-addition algorithm.

The tool returns:
- planar/non-planar decision,
- planar embedding for planar graphs,
- Kuratowski subdivision certificate for non-planar graphs.

## Scope

Input graphs are undirected simple graphs with vertices indexed from 0 to n - 1.

## Core output types

### Planar graph

- `planar = true`
- `PlanarEmbedding`
- rotation system around every vertex

### Non-planar graph

- `planar = false`
- `KuratowskiCertificate`
- original edge IDs forming a subdivision of K5 or K3,3

## Milestones

1. Graph model and tests.
2. Test graph generators.
3. DFS/preprocessing.
4. Embedding representation and validator.
5. Kuratowski verifier.
6. Boyer–Myrvold planarity decision.
7. Planar embedding extraction.
8. Kuratowski certificate extraction.
9. JSON export.
10. Visualization.