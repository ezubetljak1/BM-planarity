#include "TestSupport.hpp"

#include "bm/BmEmbeddingState.hpp"
#include "bm/DfsPreprocessor.hpp"
#include "bm/Graph.hpp"

using namespace bm;

BM_TEST(BmEmbeddingStateCopiesChildrenSortedByLowpoint) {
    Graph g(4);
    g.addEdge(0, 1);
    g.addEdge(0, 2);
    g.addEdge(2, 3);
    g.addEdge(3, 0);

    DfsPreprocessor preprocessor;
    DfsInfo dfsInfo = preprocessor.run(g);

    BmEmbeddingState state(g, dfsInfo);

    for (int v = 0; v < g.vertexCount(); ++v) {
        BM_ASSERT(state.vertexState(v).separatedDfsChildList ==
                  dfsInfo.childrenSortedByLowpoint[v]);
    }
}

BM_TEST(BmEmbeddingStateDetectsExternalActivityFromLeastAncestor) {
    Graph g(3);
    g.addEdge(0, 1);
    g.addEdge(1, 2);
    g.addEdge(2, 0);

    DfsPreprocessor preprocessor;
    DfsInfo dfsInfo = preprocessor.run(g);

    BmEmbeddingState state(g, dfsInfo);

    // During processing of vertex 1, vertex 2 is externally active
    // because it has a back edge to 0, which is an ancestor of 1.
    BM_ASSERT(state.isExternallyActive(2, 1));
}

BM_TEST(BmEmbeddingStateDoesNotMarkInactiveLeafAsExternallyActive) {
    Graph g(3);
    g.addEdge(0, 1);
    g.addEdge(1, 2);

    DfsPreprocessor preprocessor;
    DfsInfo dfsInfo = preprocessor.run(g);

    BmEmbeddingState state(g, dfsInfo);

    BM_ASSERT(!state.isExternallyActive(2, 1));
}