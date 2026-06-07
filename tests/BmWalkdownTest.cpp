#include "TestSupport.hpp"

#include "bm/BmEmbeddingState.hpp"
#include "bm/BmWalkdown.hpp"
#include "bm/BmWalkup.hpp"
#include "bm/DfsPreprocessor.hpp"
#include "bm/Graph.hpp"
#include "bm/BmExternalFaceTraversal.hpp"

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

BM_TEST(BmWalkdownEmbedsBackedgeOfTriangle) {
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

    const BmWalkdownResult result = walkdown.run(state, 0, state.rootForChild(1));

    BM_ASSERT(result.completed);
    BM_ASSERT(state.isOriginalEdgeEmbedded(edge02));
    BM_ASSERT(!state.hasBackedgeFlag(2, 0));
}

BM_TEST(BmWalkdownHandlesTreeWithoutBackedges) {
    Graph graph(4);

    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 3);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state = makeInitializedState(graph, dfsInfo);

    BmWalkdown walkdown;

    BM_ASSERT(walkdown.run(state, 0, state.rootForChild(1)).completed);
}

BM_TEST(BmWalkdownEmbedsCycleBackedge) {
    Graph graph(4);

    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 3);

    const int edge03 = graph.addEdge(0, 3);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state = makeInitializedState(graph, dfsInfo);

    BmWalkup walkup;

    walkup.run(state, 0, 3, edge03);

    BmWalkdown walkdown;

    const BmWalkdownResult result = walkdown.run(state, 0, state.rootForChild(1));

    BM_ASSERT(result.completed);
    BM_ASSERT(state.isOriginalEdgeEmbedded(edge03));
}

BM_TEST(BmWalkdownSkipsBicompWithoutPertinentSubgraph) {
    Graph graph(3);

    graph.addEdge(0, 1);
    graph.addEdge(1, 2);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state(graph, dfsInfo);

    const int root12 = state.createTreeEdgeBicomp(1, 2);

    const int edgeCountBefore = state.partialEmbedding().embeddedEdgeCount();

    BmWalkdown walkdown;

    const BmWalkdownResult result = walkdown.run(state, 1, root12);

    BM_ASSERT(result.completed);

    BM_ASSERT(state.partialEmbedding().embeddedEdgeCount() == edgeCountBefore);

    const BmBicompRoot& root = state.bicompRoot(root12);

    BmExternalFaceTraversal traversal(state.partialEmbedding());

    BmExternalFacePosition first;
    first.internalVertexId = root.internalRootVertexId;
    first.linkIndex = 0;

    BmExternalFacePosition second;
    second.internalVertexId = root.internalRootVertexId;
    second.linkIndex = 1;

    const auto firstSuccessor = traversal.successor(first);

    const auto secondSuccessor = traversal.successor(second);

    BM_ASSERT(firstSuccessor.internalVertexId == root.internalChildVertexId);

    BM_ASSERT(secondSuccessor.internalVertexId == root.internalChildVertexId);
}