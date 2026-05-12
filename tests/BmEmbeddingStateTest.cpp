#include "TestSupport.hpp"

#include "bm/BmEmbeddingState.hpp"
#include "bm/DfsPreprocessor.hpp"
#include "bm/Graph.hpp"

using namespace bm;

BM_TEST(BmEmbeddingStateInitializesSeparatedDfsChildren) {
    Graph graph(4);
    graph.addEdge(0, 1);
    graph.addEdge(0, 2);
    graph.addEdge(0, 3);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state(graph, dfsInfo);

    BM_ASSERT(state.separatedDfsChildren(0) == dfsInfo.childrenSortedByLowpoint[0]);
}

BM_TEST(BmEmbeddingStateRemovesSeparatedDfsChild) {
    Graph graph(4);
    graph.addEdge(0, 1);
    graph.addEdge(0, 2);
    graph.addEdge(0, 3);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state(graph, dfsInfo);

    state.removeSeparatedDfsChild(0, 2);

    const auto children = state.separatedDfsChildren(0);

    BM_ASSERT(children.size() == 2);
    BM_ASSERT(state.firstSeparatedDfsChild(0) != -1);
}

BM_TEST(BmEmbeddingStateDetectsExternalActivityFromLeastAncestor) {
    Graph graph(3);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 0);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state(graph, dfsInfo);

    BM_ASSERT(state.isExternallyActive(2, 1));
}

BM_TEST(BmEmbeddingStateDoesNotMarkInactiveLeafAsExternallyActive) {
    Graph graph(3);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state(graph, dfsInfo);

    BM_ASSERT(!state.isExternallyActive(2, 1));
}

BM_TEST(BmEmbeddingStateCreatesTreeEdgeBicomp) {
    Graph graph(3);
    const int edge01 = graph.addEdge(0, 1);
    const int edge12 = graph.addEdge(1, 2);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state(graph, dfsInfo);

    const int root1 = state.createTreeEdgeBicomp(0, 1);
    const int root2 = state.createTreeEdgeBicomp(1, 2);

    BM_ASSERT(root1 != -1);
    BM_ASSERT(root2 != -1);
    BM_ASSERT(root1 != root2);

    BM_ASSERT(state.bicompRootCount() == 2);

    BM_ASSERT(state.rootForChild(1) == root1);
    BM_ASSERT(state.rootForChild(2) == root2);

    BM_ASSERT(state.bicompRoot(root1).parentVertex == 0);
    BM_ASSERT(state.bicompRoot(root1).childVertex == 1);
    BM_ASSERT(state.bicompRoot(root1).treeEdgeId == edge01);

    BM_ASSERT(state.bicompRoot(root2).parentVertex == 1);
    BM_ASSERT(state.bicompRoot(root2).childVertex == 2);
    BM_ASSERT(state.bicompRoot(root2).treeEdgeId == edge12);
}

BM_TEST(BmEmbeddingStateDoesNotDuplicateTreeEdgeBicomp) {
    Graph graph(2);
    graph.addEdge(0, 1);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state(graph, dfsInfo);

    const int firstRoot = state.createTreeEdgeBicomp(0, 1);
    const int secondRoot = state.createTreeEdgeBicomp(0, 1);

    BM_ASSERT(firstRoot == secondRoot);
    BM_ASSERT(state.bicompRootCount() == 1);
}

BM_TEST(BmEmbeddingStateCreatesTreeBicompsInReverseDfiOrder) {
    Graph graph(4);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 3);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state(graph, dfsInfo);

    for (int i = dfsInfo.vertexCount - 1; i >= 0; --i) {
        const int vertex = dfsInfo.vertexAtDfsIndex[static_cast<std::size_t>(i)];

        for (int child : dfsInfo.children[static_cast<std::size_t>(vertex)]) {
            state.createTreeEdgeBicomp(vertex, child);
        }
    }

    BM_ASSERT(state.bicompRootCount() == 3);
    BM_ASSERT(state.rootForChild(1) != -1);
    BM_ASSERT(state.rootForChild(2) != -1);
    BM_ASSERT(state.rootForChild(3) != -1);
}

BM_TEST(BmEmbeddingStateTreeBicompCreatesInternalEmbeddingObjects) {
    Graph graph(2);
    const int edge01 = graph.addEdge(0, 1);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state(graph, dfsInfo);

    const int rootId = state.createTreeEdgeBicomp(0, 1);
    const BmBicompRoot& root = state.bicompRoot(rootId);

    BM_ASSERT(root.treeEdgeId == edge01);
    BM_ASSERT(root.internalRootVertexId != -1);
    BM_ASSERT(root.internalChildVertexId != -1);
    BM_ASSERT(root.embeddedTreeEdgeId != -1);
    BM_ASSERT(root.rootToChildHalfEdgeId != -1);
    BM_ASSERT(root.childToRootHalfEdgeId != -1);

    const BmPartialEmbedding& embedding = state.partialEmbedding();

    BM_ASSERT(embedding.internalVertex(root.internalRootVertexId).kind ==
              BmInternalVertexKind::BicompRoot);

    BM_ASSERT(embedding.internalVertex(root.internalChildVertexId).kind ==
              BmInternalVertexKind::Original);

    BM_ASSERT(embedding.embeddedEdge(root.embeddedTreeEdgeId).originalEdgeId == edge01);

    const auto face = embedding.externalFaceVertices(root.rootToChildHalfEdgeId, 4);

    BM_ASSERT(face.size() == 2);
    BM_ASSERT(face[0] == root.internalRootVertexId);
    BM_ASSERT(face[1] == root.internalChildVertexId);
}

BM_TEST(BmEmbeddingStateMarksBackedgeFlag) {
    Graph graph(2);
    graph.addEdge(0, 1);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state(graph, dfsInfo);

    BM_ASSERT(!state.hasBackedgeFlag(1));

    state.markBackedgeFlag(1);

    BM_ASSERT(state.hasBackedgeFlag(1));
    BM_ASSERT(state.isPertinent(1));

    state.clearBackedgeFlag(1);

    BM_ASSERT(!state.hasBackedgeFlag(1));
    BM_ASSERT(!state.isPertinent(1));
}

BM_TEST(BmEmbeddingStateMarksVisitedInStepByCurrentDfi) {
    Graph graph(3);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state(graph, dfsInfo);

    BM_ASSERT(!state.isVisitedInStep(2, 1));

    state.markVisitedInStep(2, 1);

    BM_ASSERT(state.isVisitedInStep(2, 1));
}

BM_TEST(BmEmbeddingStateAddsPertinentRoot) {
    Graph graph(2);
    graph.addEdge(0, 1);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state(graph, dfsInfo);

    const int rootId = state.createTreeEdgeBicomp(0, 1);

    BM_ASSERT(!state.hasPertinentRoots(0));

    state.addPertinentRoot(0, rootId, 0);

    BM_ASSERT(state.hasPertinentRoots(0));
    BM_ASSERT(state.firstPertinentRoot(0) == rootId);
    BM_ASSERT(state.isPertinent(0));

    state.removeFirstPertinentRoot(0);

    BM_ASSERT(!state.hasPertinentRoots(0));
}