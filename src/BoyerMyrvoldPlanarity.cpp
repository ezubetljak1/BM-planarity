#include "bm/BoyerMyrvoldPlanarity.hpp"
#include "bm/SimpleGraphValidator.hpp"

namespace bm {

PlanarityResult BoyerMyrvoldPlanarity::run(const Graph &graph) const {
    SimpleGraphValidator::validate(graph);

    DfsPreprocessor dfsPreprocessor;
    const DfsInfo dfsInfo = dfsPreprocessor.run(graph);

    BmEmbeddingState state(graph, dfsInfo);

    createInitialTreeBicomps(dfsInfo, state);

    // Placeholder until Walkup/Walkdown and recovery are implemented
    return makePlaceholderPlanarResult(graph);
}

void BoyerMyrvoldPlanarity::createInitialTreeBicomps(const DfsInfo &dfsInfo, BmEmbeddingState &state) {
    for (int dfi = dfsInfo.vertexCount - 1; dfi >= 0; --dfi) {
        const int vertex = dfsInfo.vertexAtDfsIndex[dfi];

        for (int child : dfsInfo.children[vertex])
            state.createTreeEdgeBicomp(vertex, child);
    }
}

PlanarityResult BoyerMyrvoldPlanarity::makePlaceholderPlanarResult(const Graph &graph) {
    PlanarEmbedding embedding;
    embedding.clockwiseEdgesAroundVertex.resize(graph.vertexCount());

    PlanarityResult result;
    result.planar = true;
    result.embedding = embedding;
    result.certificate = std::nullopt;

    return result;
}

} // namespace bm