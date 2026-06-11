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

    static PlanarityResult makePlanarResult(PlanarEmbedding embedding);
    static PlanarityResult makePlaceholderNonPlanarResult();
};

} // namespace bm