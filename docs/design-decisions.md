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


---

## DD-006 — Recover and validate a combinatorial embedding before drawing

### Decision
After a successful planarity decision, orient the remaining bicomps, merge their virtual root
copies into the corresponding original vertices and return a rotation system of original edge IDs.
Validate that rotation system independently before exposing it as the public planar embedding.

### Reason
The edge-addition phase only determines and incrementally constructs a partial embedding. Lazy flip
signs and separated bicomps must be resolved during post-processing. A rotation-system validator
provides an independent check before later visualization code relies on the result.

### Consequences
- `BmEmbeddingRecovery` follows the post-processing split used by the reference implementation:
  orient bicomps, join remaining bicomps, then export rotations.
- `PlanarEmbeddingValidator` checks endpoint occurrences and Euler's relation per connected
  component.
- Shortcut links remain traversal-only data and never appear in the exported rotation system.


---

## DD-007 — Validate Kuratowski subdivisions independently before extraction

### Decision
Implement a linear `KuratowskiCertificateVerifier` before integrating the reference-style A-E
Kuratowski isolator into the non-planar failure path.

### Reason
A certificate extractor must not be trusted merely because the decision core reports
`NONPLANAR`. The selected original edge IDs must independently suppress to exactly `K5` or
`K3,3`. A verifier also enables broad regression testing with witnesses obtained from NetworkX.

### Consequences
- The verifier accepts only original edge IDs and never shortcut traversal links.
- Degree-2 subdivision paths are suppressed logically without mutating the input graph.
- The verifier runs in `O(n + m)` time.
- Automatic extraction remains a separate next phase based on the Boyer-Myrvold non-planarity
  minors A-E and preserved Walkdown failure context.

---

## DD-008 — Preserve Walkdown failure context and separate real-face traversal

### Decision
When Walkdown reports non-planarity, preserve the local failure context needed by the Kuratowski
isolator. Introduce `BmRealExternalFaceTraversal` for certificate extraction and keep it separate
from the optimized shortcut traversal used by Walkup and Walkdown.

### Reason
The Boyer-Myrvold isolator must inspect the real external face of the principal bicomp. Shortcut
links deliberately skip inactive paths and are therefore unsuitable for marking the original
subdivision paths that form a certificate. The reference implementation explicitly ignores
external-face optimization links while initializing the obstruction context.

### Consequences
- `BmWalkdownResult` stores a structured `BmWalkdownFailure` snapshot.
- `BmRealExternalFaceTraversal` follows actual half-edge anchors and ignores shortcut links.
- `BmKuratowskiExtractionContextBuilder` initializes the principal root, stopping vertices and the
  first pertinent vertex needed by the later minor A-E isolator.
- Certificate extraction can continue without rerunning the decision algorithm or reconstructing a
  lost conflict state.

---

## DD-009 — Extract Kuratowski subdivisions from preserved failure state

### Decision
After the decision core reports `NONPLANAR`, isolate a Kuratowski subdivision directly from the
preserved Walkdown failure snapshot. Classify the obstruction as Boyer-Myrvold Minor A-E, mark the
required real external-face paths, DFS paths and original unembedded back edges, and expose only
stable original graph edge IDs in the public certificate.

### Reason
Rerunning planarity tests while deleting edges would be substantially more expensive and would
ignore the linear post-failure isolator described by Boyer and Myrvold. Shortcut traversal links are
optimization metadata and must not leak into the public witness.

### Consequences
- `BmKuratowskiExtractor` dispatches reference-style Minor A-E isolators.
- `BmRealExternalFaceTraversal` is used when marking certificate paths.
- `KuratowskiCertificateVerifier` independently validates every extracted witness.
- NetworkX-backed regression suppresses subdivision paths independently in Python.

---

## DD-010 — Reduce very dense simple inputs before certified isolation

### Decision
When a simple graph with `n >= 3` has more than `3n - 5` edges, run the certifying Boyer-Myrvold
core on an arbitrary `3n - 5`-edge subgraph and map the resulting witness back to original stable
edge IDs.

### Reason
Every simple planar graph has at most `3n - 6` edges. Therefore every simple `3n - 5`-edge
subgraph is already non-planar and contains a Kuratowski subdivision. Bounding the graph passed to
the certifying core preserves the intended linear-size working set.

### Consequences
- Input validation still costs `O(n + m)` because all input edges must be read and checked.
- The Boyer-Myrvold certifying core processes at most `3n - 5` edges.
- Public certificate edge IDs always refer to the original graph.
