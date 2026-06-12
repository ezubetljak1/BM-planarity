#pragma once

#include "bm/Graph.hpp"
#include "bm/PlanarityResult.hpp"

#include <vector>

namespace bm::layout {

struct VertexPosition {
    double x = 0.0;
    double y = 0.0;
};

struct PlanarLayout {
    std::vector<VertexPosition> positionsByVertex;
};

class OgdfPlanarLayoutAdapter {
public:

    static PlanarLayout compute(const Graph& graph, const PlanarEmbedding& embedding);

};

} // namespace bm::layout