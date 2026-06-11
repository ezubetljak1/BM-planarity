#include "TestSupport.hpp"

#include "bm/BmEmbeddingOperations.hpp"
#include "bm/BmEmbeddingState.hpp"
#include "bm/BmEmbeddingValidator.hpp"
#include "bm/BmWalkdown.hpp"
#include "bm/BmWalkup.hpp"
#include "bm/DfsPreprocessor.hpp"
#include "bm/Graph.hpp"

#include <stdexcept>
#include <vector>

using namespace bm;

namespace {

BmEmbeddingState makeInitializedState(const Graph& graph, const DfsInfo& dfsInfo) {
    BmEmbeddingState state(graph, dfsInfo);

    for (int dfi = dfsInfo.vertexCount - 1; dfi >= 0; --dfi) {
        const int vertex = dfsInfo.vertexAtDfsIndex[dfi];

        for (int child : dfsInfo.children[vertex]) {
            state.createTreeEdgeBicomp(vertex, child);
        }
    }

    return state;
}

} // namespace

BM_TEST(BmEmbeddingValidatorAcceptsInitialTreeBicomps) {
    Graph graph(4);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 3);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state = makeInitializedState(graph, dfsInfo);

    BmEmbeddingValidator::validateState(state);
}

BM_TEST(BmEmbeddingValidatorAcceptsTriangleAfterWalkdown) {
    Graph graph(3);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    const int edge02 = graph.addEdge(0, 2);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state = makeInitializedState(graph, dfsInfo);

    BmWalkup walkup;
    walkup.run(state, 0, 2, edge02);

    BmWalkdown walkdown;
    BM_ASSERT(walkdown.run(state, 0, state.rootForChild(1)).completed);

    BmEmbeddingValidator::validateState(state);
}

BM_TEST(BmEmbeddingValidatorAcceptsMergedBicompAdjacencyStructure) {
    Graph graph(3);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state(graph, dfsInfo);
    state.createTreeEdgeBicomp(0, 1);
    const int root12 = state.createTreeEdgeBicomp(1, 2);
    state.addPertinentRoot(1, root12, 0);

    std::vector<BmMergeFrame> mergeStack;
    mergeStack.push_back({1, 0, root12, 1});

    BmEmbeddingOperations::mergeTopBiconnectedComponent(state, mergeStack);

    BmEmbeddingValidator::validatePartialEmbedding(state.partialEmbedding());
}

BM_TEST(BmEmbeddingValidatorRejectsBrokenTwinRelation) {
    BmPartialEmbedding embedding(2);
    const BmTreeBicompEmbedding tree = embedding.createTreeEdgeBicomp(0, 0, 1, 7);

    embedding.halfEdge(tree.rootToChildHalfEdgeId).twin = tree.rootToChildHalfEdgeId;

    bool threw = false;

    try {
        BmEmbeddingValidator::validatePartialEmbedding(embedding);
    } catch (const std::logic_error&) {
        threw = true;
    }

    BM_ASSERT(threw);
}
