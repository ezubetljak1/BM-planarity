#pragma once

#include "bm/Graph.hpp"
#include "bm/PlanarityResult.hpp"

namespace bm {

class PlanarEmbeddingValidator {
public:
    static void validate(const Graph& graph, const PlanarEmbedding& embedding);
};

} // namespace bm
