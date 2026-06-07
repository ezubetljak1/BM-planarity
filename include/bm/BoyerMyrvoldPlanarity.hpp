#pragma once

#include "bm/BmEmbeddingState.hpp"
#include "bm/DfsPreprocessor.hpp"
#include "bm/Graph.hpp"
#include "bm/PlanarityResult.hpp"

namespace bm {

class BoyerMyrvoldPlanarity {
public:
    PlanarityResult run(const Graph& graph) const;

private:
    static void createInitialTreeBicomps(const DfsInfo& dfsInfo, BmEmbeddingState& state);

    static PlanarityResult makePlaceholderPlanarResult(const Graph& graph);
    static PlanarityResult makePlaceholderNonPlanarResult();
};

} // namespace bm