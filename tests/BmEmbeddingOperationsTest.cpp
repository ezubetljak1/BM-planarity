#include "TestSupport.hpp"

#include "bm/BmEmbeddingOperations.hpp"
#include "bm/BmEmbeddingState.hpp"
#include "bm/DfsPreprocessor.hpp"
#include "bm/Graph.hpp"

#include <vector>

using namespace bm;

BM_TEST(BmEmbeddingOperationsMergesVirtualRootIntoRealVertexWithoutFlip) {
    Graph graph(3);

    graph.addEdge(0, 1);
    graph.addEdge(1, 2);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state(graph, dfsInfo);

    state.createTreeEdgeBicomp(0, 1);
    const int root12 = state.createTreeEdgeBicomp(1, 2);

    state.addPertinentRoot(1, root12, 0);

    const BmBicompRoot rootBefore = state.bicompRoot(root12);

    std::vector<BmMergeFrame> mergeStack;

    BmMergeFrame frame;
    frame.cutVertex = 1;
    frame.cutVertexIncomingLink = 0;
    frame.rootId = root12;
    frame.rootOutgoingLink = 1;

    mergeStack.push_back(frame);

    BmEmbeddingOperations::mergeTopBiconnectedComponent(state, mergeStack);

    const BmBicompRoot& rootAfter = state.bicompRoot(root12);

    BM_ASSERT(!rootAfter.active);
    BM_ASSERT(!state.hasPertinentRoots(1));
    BM_ASSERT(state.separatedDfsChildren(1).empty());

    const BmPartialEmbedding& embedding = state.partialEmbedding();

    const int realVertex1 = embedding.originalInternalVertex(1);

    BM_ASSERT(embedding.halfEdge(rootBefore.rootToChildHalfEdgeId).from == realVertex1);

    BM_ASSERT(embedding.halfEdge(rootBefore.childToRootHalfEdgeId).to == realVertex1);

    BM_ASSERT(embedding.adjacencyEmpty(rootBefore.internalRootVertexId));

    BM_ASSERT(embedding.embeddedEdge(rootBefore.embeddedTreeEdgeId).sign == 1);
}

BM_TEST(BmEmbeddingOperationsFlipsVirtualRootWhenDirectionsMatch) {
    Graph graph(3);

    graph.addEdge(0, 1);
    graph.addEdge(1, 2);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state(graph, dfsInfo);

    state.createTreeEdgeBicomp(0, 1);
    const int root12 = state.createTreeEdgeBicomp(1, 2);

    state.addPertinentRoot(1, root12, 0);

    const BmBicompRoot rootBefore = state.bicompRoot(root12);

    std::vector<BmMergeFrame> mergeStack;

    BmMergeFrame frame;
    frame.cutVertex = 1;
    frame.cutVertexIncomingLink = 0;
    frame.rootId = root12;
    frame.rootOutgoingLink = 0;

    mergeStack.push_back(frame);

    BmEmbeddingOperations::mergeTopBiconnectedComponent(state, mergeStack);

    const BmBicompRoot& rootAfter = state.bicompRoot(root12);

    BM_ASSERT(!rootAfter.active);
    BM_ASSERT(rootAfter.rootEdgeSign == -1);

    BM_ASSERT(state.partialEmbedding().embeddedEdge(rootBefore.embeddedTreeEdgeId).sign == -1);
}