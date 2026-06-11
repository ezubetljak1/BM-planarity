#include "bm/BoyerMyrvoldPlanarity.hpp"
#include "bm/SimpleGraphValidator.hpp"
#include "bm/BmWalkdown.hpp"
#include "bm/BmWalkup.hpp"

namespace bm {

PlanarityResult BoyerMyrvoldPlanarity::run(const Graph& graph) const {
    SimpleGraphValidator::validate(graph);

    DfsPreprocessor dfsPreprocessor;
    const DfsInfo dfsInfo = dfsPreprocessor.run(graph);

    BmEmbeddingState state(graph, dfsInfo);

    BmWalkup walkup;
    BmWalkdown walkdown;

    for (int dfi = dfsInfo.vertexCount - 1; dfi >= 0; --dfi) {
        const int vertex = dfsInfo.vertexAtDfsIndex[dfi];

        for (int child : dfsInfo.children[vertex]) {
            state.createTreeEdgeBicomp(vertex, child);
        }

        for (int backEdgeIndex : dfsInfo.backEdgeIndicesFromAncestor[vertex]) {
            const DfsBackEdge& backEdge = dfsInfo.backEdges[backEdgeIndex];

            walkup.run(state, vertex, backEdge.descendant, backEdge.edgeId);
        }

        for (int child : dfsInfo.children[vertex]) {
            // Walkdown is required only for a DFS subtree that became
            // pertinent during the Walkup phase of the current vertex.
            if (!state.hasPertinentRoots(child)) {
                continue;
            }

            const int rootId = state.rootForChild(child);

            const BmWalkdownResult result = walkdown.run(state, vertex, rootId);

            if (!result.completed) {
                return makePlaceholderNonPlanarResult();
            }
        }

        for (int backEdgeIndex : dfsInfo.backEdgeIndicesFromAncestor[vertex]) {
            const DfsBackEdge& backEdge = dfsInfo.backEdges[backEdgeIndex];

            if (!state.isOriginalEdgeEmbedded(backEdge.edgeId)) {
                return makePlaceholderNonPlanarResult();
            }
        }
    }

    // Recovery dolazi kao naredni blok.
    return makePlaceholderPlanarResult(graph);
}

void BoyerMyrvoldPlanarity::createInitialTreeBicomps(const DfsInfo& dfsInfo,
                                                     BmEmbeddingState& state) {
    for (int dfi = dfsInfo.vertexCount - 1; dfi >= 0; --dfi) {
        const int vertex = dfsInfo.vertexAtDfsIndex[dfi];

        for (int child : dfsInfo.children[vertex])
            state.createTreeEdgeBicomp(vertex, child);
    }
}

PlanarityResult BoyerMyrvoldPlanarity::makePlaceholderPlanarResult(const Graph& graph) {
    PlanarEmbedding embedding;
    embedding.clockwiseEdgesAroundVertex.resize(graph.vertexCount());

    PlanarityResult result;
    result.planar = true;
    result.embedding = embedding;
    result.certificate = std::nullopt;

    return result;
}

PlanarityResult BoyerMyrvoldPlanarity::makePlaceholderNonPlanarResult() {
    PlanarityResult result;
    result.planar = false;
    result.embedding = std::nullopt;
    result.certificate = std::nullopt;

    return result;
}

} // namespace bm