#include "bm/SimpleGraphValidator.hpp"

#include <stdexcept>
#include <vector>

namespace bm {

void SimpleGraphValidator::validate(const Graph& graph) {
    const int n = graph.vertexCount();

    std::vector<int> seenNeighbor(n, -1);

    const auto& adjacency = graph.adjacencyEdgeIds();

    for (int u = 0; u < n; ++u) {
        for (int edgeId : adjacency[u]) {
            const int v = graph.opposite(edgeId, u);

            if (u == v)
                throw std::invalid_argument("Self-loops are not supported.");

            if (seenNeighbor[v] == u)
                throw std::invalid_argument("Parallel edges are not supported.");

            seenNeighbor[v] = u;
        }
    }
}

} // namespace bm