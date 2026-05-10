#include "TestSupport.hpp"

#include "bm/Graph.hpp"

using namespace bm;

BM_TEST(GraphCreatesVerticesAndEdges) {
    Graph g(3);

    const int e0 = g.addEdge(0, 1);
    const int e1 = g.addEdge(1, 2);

    BM_ASSERT(g.vertexCount() == 3);
    BM_ASSERT(g.edgeCount() == 2);

    BM_ASSERT(e0 == 0);
    BM_ASSERT(e1 == 1);

    BM_ASSERT(g.edge(0).u == 0);
    BM_ASSERT(g.edge(0).v == 1);

    BM_ASSERT(g.edge(1).u == 1);
    BM_ASSERT(g.edge(1).v == 2);
}

BM_TEST(GraphStoresAdjacencyEdgeIds) {
    Graph g(3);

    const int e0 = g.addEdge(0, 1);
    const int e1 = g.addEdge(0, 2);

    const auto& adj = g.adjacencyEdgeIds();

    BM_ASSERT(adj[0].size() == 2);
    BM_ASSERT(adj[1].size() == 1);
    BM_ASSERT(adj[2].size() == 1);

    BM_ASSERT(adj[0][0] == e0);
    BM_ASSERT(adj[0][1] == e1);
    BM_ASSERT(adj[1][0] == e0);
    BM_ASSERT(adj[2][0] == e1);
}

BM_TEST(GraphRejectsInvalidVertex) {
    Graph g(3);

    bool threw = false;

    try {
        g.addEdge(0, 5);
    } catch (const std::out_of_range&) {
        threw = true;
    }

    BM_ASSERT(threw);
}

BM_TEST(GraphRejectsSelfLoop) {
    Graph g(3);

    bool threw = false;

    try {
        g.addEdge(1, 1);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    BM_ASSERT(threw);
}