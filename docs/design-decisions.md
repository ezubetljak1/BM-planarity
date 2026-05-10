# Design Decisions

## DD-001 — Use stable original edge IDs

### Decision
Every edge receives a stable original edge ID at insertion time.

### Reason
The Kuratowski certificate and visualization must refer back to the original input graph.

### Consequences
All internal structures must preserve or map back to original edge IDs. 

---

## DD-002 — Implement verifier before full certificate extraction

### Decision
An independent Kuratowski verifier will be implemented before relying on certificate extraction.

### Reason
The algorithm must not only return a set of edges; the returned set must be automatically checked as a K5 or K3,3 subdivision.

### Consequences
Certificate extraction can be developed incrementally and tested safely. 

---

## DD-003 — Separate embedding from drawing coordinates

### Decision
The algorithm first produces a combinatorial embedding. Coordinate generation is implemented as a separate visualization step.

### Reason
A planar embedding is a topological result, while drawing coordinates require an additional layout algorithm.

### Consequences
The first implementation target is a rotation system, not a polished drawing.