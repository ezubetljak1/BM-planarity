#pragma once

#include "bm/BmEmbeddingState.hpp"
#include "bm/BmPlanarityProfiling.hpp"
#include "bm/DfsPreprocessor.hpp"
#include "bm/Graph.hpp"
#include "bm/PlanarityResult.hpp"

namespace bm {

class BoyerMyrvoldPlanarity {
public:
    PlanarityResult run(const Graph& graph) const;
    BmProfiledPlanarityResult runProfiled(const Graph& graph) const;

private:
    PlanarityResult runInternal(const Graph& graph, BmPlanarityPhaseTimings* timings) const;

    static PlanarityResult makePlanarResult(PlanarEmbedding embedding);
    static PlanarityResult makeNonPlanarResult(KuratowskiCertificate certificate);
};

} // namespace bm
