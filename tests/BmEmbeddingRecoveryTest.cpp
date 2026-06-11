#include "TestSupport.hpp"

#include "bm/BmEmbeddingRecovery.hpp"
#include "bm/BmEmbeddingState.hpp"
#include "bm/BmEmbeddingValidator.hpp"
#include "bm/DfsPreprocessor.hpp"
#include "bm/Graph.hpp"
#include "bm/PlanarEmbeddingValidator.hpp"

using namespace bm;

namespace {

BmEmbeddingState makeTreeState(const Graph& graph, const DfsInfo& dfsInfo) {
    BmEmbeddingState state(graph, dfsInfo);

    for (int dfi = dfsInfo.vertexCount - 1; dfi >= 0; --dfi) {
        const int vertex = dfsInfo.vertexAtDfsIndex[static_cast<std::size_t>(dfi)];

        for (int child : dfsInfo.children[static_cast<std::size_t>(vertex)]) {
            state.createTreeEdgeBicomp(vertex, child);
        }
    }

    return state;
}

} // namespace

BM_TEST(BmEmbeddingRecoveryJoinsRemainingTreeBicomps) {
    Graph graph(4);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 3);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);
    BmEmbeddingState state = makeTreeState(graph, dfsInfo);

    const PlanarEmbedding embedding = BmEmbeddingRecovery::recover(state);

    BM_ASSERT(embedding.clockwiseEdgesAroundVertex.size() == 4);
    BM_ASSERT(embedding.clockwiseEdgesAroundVertex[0].size() == 1);
    BM_ASSERT(embedding.clockwiseEdgesAroundVertex[1].size() == 2);
    BM_ASSERT(embedding.clockwiseEdgesAroundVertex[2].size() == 2);
    BM_ASSERT(embedding.clockwiseEdgesAroundVertex[3].size() == 1);

    for (int rootId = 0; rootId < state.bicompRootCount(); ++rootId) {
        BM_ASSERT(!state.bicompRoot(rootId).active);
        BM_ASSERT(state.partialEmbedding().adjacencyEmpty(
            state.bicompRoot(rootId).internalRootVertexId));
    }

    PlanarEmbeddingValidator::validate(graph, embedding);
    BmEmbeddingValidator::validateState(state);
}

BM_TEST(BmEmbeddingRecoveryReturnsValidEmbeddingForDisconnectedForest) {
    Graph graph(6);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(3, 4);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);
    BmEmbeddingState state = makeTreeState(graph, dfsInfo);

    const PlanarEmbedding embedding = BmEmbeddingRecovery::recover(state);

    BM_ASSERT(embedding.clockwiseEdgesAroundVertex.size() == 6);
    BM_ASSERT(embedding.clockwiseEdgesAroundVertex[5].empty());
    PlanarEmbeddingValidator::validate(graph, embedding);
    BmEmbeddingValidator::validateState(state);
}
