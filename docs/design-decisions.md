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
---

## DD-004 — Keep external-face shortcuts separate from real half-edges

### Decision
Walkdown stores root-to-stopping-vertex shortcuts in `externalFaceNeighbors` instead of creating
fake embedded edges.

### Reason
The shortcuts exist only to avoid repeated traversal of inactive external-face paths. They are not
part of the input graph and must not appear in the recovered rotation system or a Kuratowski
certificate.

### Consequences
- `externalFaceHalfEdges` remain adjacency anchors for real embedded edges.
- `externalFaceNeighbors` are the source of truth for Walkup and Walkdown external-face traversal.
- Recovery must traverse real adjacency lists rather than shortcut links.

---

## DD-005 — Validate mutable embedding invariants independently

### Decision
Use `BmEmbeddingValidator` in tests to validate the partial embedding and the BM state after
low-level mutations.

### Reason
Walkup, Walkdown, merge and lazy-flip operations mutate several linked structures at once. A wrong
link can remain hidden until a later graph triggers the corrupted path.

### Consequences
Regression tests validate twin relations, circular adjacency links, bicomp-root metadata and
original-edge mappings independently of the final planarity decision.
