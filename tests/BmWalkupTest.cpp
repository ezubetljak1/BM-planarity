#include "TestSupport.hpp"

#include "bm/BmEmbeddingState.hpp"
#include "bm/BmWalkup.hpp"
#include "bm/DfsPreprocessor.hpp"
#include "bm/Graph.hpp"

#include <stdexcept>

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

BM_TEST(BmWalkupMarksBackedgeEndpointAsPertinent) {
    Graph graph(3);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(0, 2);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state = makeInitializedState(graph, dfsInfo);

    BmWalkup walkup;
    walkup.run(state, 0, 2);

    BM_ASSERT(state.hasBackedgeFlag(2));
    BM_ASSERT(state.isPertinent(2));
}

BM_TEST(BmWalkupAddsIntermediateRootToPertinentRoots) {
    Graph graph(3);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(0, 2);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state = makeInitializedState(graph, dfsInfo);

    const int rootForChild2 = state.rootForChild(2);

    BmWalkup walkup;
    walkup.run(state, 0, 2);

    BM_ASSERT(state.hasPertinentRoots(1));
    BM_ASSERT(state.firstPertinentRoot(1) == rootForChild2);

    // The root 0^1 belongs to the current vertex side and should not be
    // inserted into pertinentRoots[0].
    BM_ASSERT(!state.hasPertinentRoots(0));
}

BM_TEST(BmWalkupAddsMultipleRootsAlongTreePath) {
    Graph graph(4);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 3);
    graph.addEdge(0, 3);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state = makeInitializedState(graph, dfsInfo);

    const int rootForChild3 = state.rootForChild(3);
    const int rootForChild2 = state.rootForChild(2);

    BmWalkup walkup;
    walkup.run(state, 0, 3);

    BM_ASSERT(state.hasBackedgeFlag(3));

    BM_ASSERT(state.hasPertinentRoots(2));
    BM_ASSERT(state.firstPertinentRoot(2) == rootForChild3);

    BM_ASSERT(state.hasPertinentRoots(1));
    BM_ASSERT(state.firstPertinentRoot(1) == rootForChild2);

    BM_ASSERT(!state.hasPertinentRoots(0));
}

BM_TEST(BmWalkupVisitedStopsDuplicatePertinentRootInsertion) {
    Graph graph(4);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 3);
    graph.addEdge(0, 3);
    graph.addEdge(0, 2);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state = makeInitializedState(graph, dfsInfo);

    BmWalkup walkup;
    walkup.run(state, 0, 3);

    const int sizeBefore = static_cast<int>(
        state.vertexState(1).pertinentRoots.size()
    );

    walkup.run(state, 0, 2);

    const int sizeAfter = static_cast<int>(
        state.vertexState(1).pertinentRoots.size()
    );

    BM_ASSERT(state.hasBackedgeFlag(3));
    BM_ASSERT(state.hasBackedgeFlag(2));

    BM_ASSERT(sizeBefore == 1);
    BM_ASSERT(sizeAfter == sizeBefore);
}

BM_TEST(BmWalkupWorksWhenCurrentVertexIsNotDfsRoot) {
    Graph graph(4);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 3);
    graph.addEdge(1, 3);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state = makeInitializedState(graph, dfsInfo);

    const int rootForChild3 = state.rootForChild(3);

    BmWalkup walkup;
    walkup.run(state, 1, 3);

    BM_ASSERT(state.hasBackedgeFlag(3));

    BM_ASSERT(state.hasPertinentRoots(2));
    BM_ASSERT(state.firstPertinentRoot(2) == rootForChild3);

    BM_ASSERT(!state.hasPertinentRoots(1));
    BM_ASSERT(!state.hasPertinentRoots(0));
}

BM_TEST(BmWalkupMarksInternalVerticesVisitedForCurrentStep) {
    Graph graph(4);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 3);
    graph.addEdge(0, 3);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state = makeInitializedState(graph, dfsInfo);

    const int original3 = state.partialEmbedding().originalInternalVertex(3);
    const int rootForChild3 = state.rootForChild(3);
    const int rootForChild2 = state.rootForChild(2);

    const int internalRoot3 = state.bicompRoot(rootForChild3).internalRootVertexId;
    const int internalRoot2 = state.bicompRoot(rootForChild2).internalRootVertexId;

    BmWalkup walkup;
    walkup.run(state, 0, 3);

    BM_ASSERT(state.isInternalVertexVisitedInStep(original3, 0));
    BM_ASSERT(state.isInternalVertexVisitedInStep(internalRoot3, 0));
    BM_ASSERT(state.isInternalVertexVisitedInStep(internalRoot2, 0));
}

BM_TEST(BmWalkupDoesNotMarkCurrentVertexAsVisitedWhenItStops) {
    Graph graph(3);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(0, 2);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state = makeInitializedState(graph, dfsInfo);

    const int original0 = state.partialEmbedding().originalInternalVertex(0);

    BmWalkup walkup;
    walkup.run(state, 0, 2);

    BM_ASSERT(!state.isInternalVertexVisitedInStep(original0, 0));
}

BM_TEST(BmWalkupVisitedIsScopedByCurrentVertexDfi) {
    Graph graph(4);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 3);
    graph.addEdge(0, 3);
    graph.addEdge(1, 3);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state = makeInitializedState(graph, dfsInfo);

    const int original3 = state.partialEmbedding().originalInternalVertex(3);

    BmWalkup walkup;
    walkup.run(state, 0, 3);

    BM_ASSERT(state.isInternalVertexVisitedInStep(original3, 0));
    BM_ASSERT(!state.isInternalVertexVisitedInStep(original3, 1));

    walkup.run(state, 1, 3);

    BM_ASSERT(state.isInternalVertexVisitedInStep(original3, 1));
}

BM_TEST(BmWalkupOrdersInternallyActiveRootBeforeExternallyActiveRoot) {
    Graph graph(5);

    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 3);
    graph.addEdge(2, 4);

    // Makes child 3 externally active while processing current vertex 1,
    // because lowpoint(3) reaches ancestor 0.
    graph.addEdge(0, 3);

    // Back edges from current vertex 1 to descendants 3 and 4.
    graph.addEdge(1, 3);
    graph.addEdge(1, 4);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state = makeInitializedState(graph, dfsInfo);

    const int rootForChild3 = state.rootForChild(3);
    const int rootForChild4 = state.rootForChild(4);

    BM_ASSERT(state.isRootExternallyActive(rootForChild3, 1));
    BM_ASSERT(!state.isRootExternallyActive(rootForChild4, 1));

    BmWalkup walkup;

    // Add externally active root first.
    walkup.run(state, 1, 3);

    // Then add internally active root. It should be pushed to the front.
    walkup.run(state, 1, 4);

    BM_ASSERT(state.hasPertinentRoots(2));
    BM_ASSERT(state.vertexState(2).pertinentRoots.size() == 2);

    BM_ASSERT(state.vertexState(2).pertinentRoots[0] == rootForChild4);
    BM_ASSERT(state.vertexState(2).pertinentRoots[1] == rootForChild3);
}

BM_TEST(BmWalkupHandlesBackedgesInDifferentBranches) {
    Graph graph(6);

    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 4);
    graph.addEdge(1, 3);
    graph.addEdge(3, 5);

    graph.addEdge(1, 4);
    graph.addEdge(1, 5);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state = makeInitializedState(graph, dfsInfo);

    const int rootForChild4 = state.rootForChild(4);
    const int rootForChild5 = state.rootForChild(5);

    BmWalkup walkup;

    walkup.run(state, 1, 4);
    walkup.run(state, 1, 5);

    BM_ASSERT(state.hasBackedgeFlag(4));
    BM_ASSERT(state.hasBackedgeFlag(5));

    BM_ASSERT(state.hasPertinentRoots(2));
    BM_ASSERT(state.firstPertinentRoot(2) == rootForChild4);

    BM_ASSERT(state.hasPertinentRoots(3));
    BM_ASSERT(state.firstPertinentRoot(3) == rootForChild5);

    BM_ASSERT(!state.hasPertinentRoots(1));
}

BM_TEST(BmWalkupRejectsInvalidCurrentVertex) {
    Graph graph(3);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state = makeInitializedState(graph, dfsInfo);

    BmWalkup walkup;

    bool threw = false;

    try {
        walkup.run(state, -1, 2);
    } catch (const std::out_of_range&) {
        threw = true;
    }

    BM_ASSERT(threw);
}

BM_TEST(BmWalkupRejectsInvalidDescendantVertex) {
    Graph graph(3);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BmEmbeddingState state = makeInitializedState(graph, dfsInfo);

    BmWalkup walkup;

    bool threw = false;

    try {
        walkup.run(state, 0, 10);
    } catch (const std::out_of_range&) {
        threw = true;
    }

    BM_ASSERT(threw);
}