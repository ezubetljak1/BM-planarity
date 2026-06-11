#include "TestSupport.hpp"

#include "bm/DfsPreprocessor.hpp"
#include "bm/Graph.hpp"

using namespace bm;

BM_TEST(DfsPreprocessorHandlesEmptyGraph) {
    Graph g(0);

    DfsPreprocessor preprocessor;
    DfsInfo info = preprocessor.run(g);

    BM_ASSERT(info.vertexCount == 0);
    BM_ASSERT(info.roots.empty());
    BM_ASSERT(info.treeEdgeIds.empty());
    BM_ASSERT(info.backEdges.empty());
}

BM_TEST(DfsPreprocessorHandlesSingleVertex) {
    Graph g(1);

    DfsPreprocessor preprocessor;
    DfsInfo info = preprocessor.run(g);

    BM_ASSERT(info.vertexCount == 1);
    BM_ASSERT(info.roots.size() == 1);
    BM_ASSERT(info.roots[0] == 0);
    BM_ASSERT(info.parent[0] == -1);
    BM_ASSERT(info.dfsIndex[0] == 0);
    BM_ASSERT(info.lowpointDfi[0] == 0);
}

BM_TEST(DfsPreprocessorClassifiesPathEdgesAsTreeEdges) {
    Graph g(4);
    const int e0 = g.addEdge(0, 1);
    const int e1 = g.addEdge(1, 2);
    const int e2 = g.addEdge(2, 3);

    DfsPreprocessor preprocessor;
    DfsInfo info = preprocessor.run(g);

    BM_ASSERT(info.roots.size() == 1);
    BM_ASSERT(info.treeEdgeIds.size() == 3);
    BM_ASSERT(info.backEdges.empty());

    BM_ASSERT(info.parent[1] == 0);
    BM_ASSERT(info.parent[2] == 1);
    BM_ASSERT(info.parent[3] == 2);

    BM_ASSERT(info.parentEdgeId[1] == e0);
    BM_ASSERT(info.parentEdgeId[2] == e1);
    BM_ASSERT(info.parentEdgeId[3] == e2);
}

BM_TEST(DfsPreprocessorDetectsBackEdgeInCycle) {
    Graph g(3);
    g.addEdge(0, 1);
    g.addEdge(1, 2);
    const int closingEdge = g.addEdge(2, 0);

    DfsPreprocessor preprocessor;
    DfsInfo info = preprocessor.run(g);

    BM_ASSERT(info.roots.size() == 1);
    BM_ASSERT(info.treeEdgeIds.size() == 2);
    BM_ASSERT(info.backEdges.size() == 1);

    const DfsBackEdge& backEdge = info.backEdges[0];

    BM_ASSERT(backEdge.edgeId == closingEdge);
    BM_ASSERT(backEdge.ancestor == 0);
    BM_ASSERT(backEdge.descendant == 2);

    BM_ASSERT(info.leastAncestorVertex[2] == 0);
    BM_ASSERT(info.lowpointDfi[2] == info.dfsIndex[0]);
    BM_ASSERT(info.lowpointDfi[1] == info.dfsIndex[0]);
}

BM_TEST(DfsPreprocessorHandlesDisconnectedGraphAsForest) {
    Graph g(5);
    g.addEdge(0, 1);
    g.addEdge(3, 4);

    DfsPreprocessor preprocessor;
    DfsInfo info = preprocessor.run(g);

    BM_ASSERT(info.roots.size() == 3);
    BM_ASSERT(info.roots[0] == 0);
    BM_ASSERT(info.roots[1] == 2);
    BM_ASSERT(info.roots[2] == 3);

    BM_ASSERT(info.parent[0] == -1);
    BM_ASSERT(info.parent[2] == -1);
    BM_ASSERT(info.parent[3] == -1);

    BM_ASSERT(info.componentId[0] == info.componentId[1]);
    BM_ASSERT(info.componentId[3] == info.componentId[4]);
    BM_ASSERT(info.componentId[2] != info.componentId[0]);
    BM_ASSERT(info.componentId[2] != info.componentId[3]);
}
BM_TEST(DfsPreprocessorHandlesDeepPathWithoutRecursion) {
    constexpr int vertexCount = 100000;

    Graph graph(vertexCount);

    for (int vertex = 0; vertex + 1 < vertexCount; ++vertex) {
        graph.addEdge(vertex, vertex + 1);
    }

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BM_ASSERT(dfsInfo.vertexCount == vertexCount);
    BM_ASSERT(dfsInfo.treeEdgeIds.size() == static_cast<std::size_t>(vertexCount - 1));
    BM_ASSERT(dfsInfo.backEdges.empty());
    BM_ASSERT(dfsInfo.parent[vertexCount - 1] == vertexCount - 2);
}
