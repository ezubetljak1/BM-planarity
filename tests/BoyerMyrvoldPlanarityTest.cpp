#include "TestSupport.hpp"

#include "bm/BoyerMyrvoldPlanarity.hpp"
#include "bm/Graph.hpp"

#include <stdexcept>

using namespace bm;

BM_TEST(BoyerMyrvoldRunHandlesEmptyGraph) {
    Graph graph(0);

    BoyerMyrvoldPlanarity algorithm;
    const PlanarityResult result = algorithm.run(graph);

    BM_ASSERT(result.planar);
    BM_ASSERT(result.embedding.has_value());
    BM_ASSERT(!result.certificate.has_value());
    BM_ASSERT(result.embedding->clockwiseEdgesAroundVertex.empty());
}

BM_TEST(BoyerMyrvoldRunHandlesSingleVertexGraph) {
    Graph graph(1);

    BoyerMyrvoldPlanarity algorithm;
    const PlanarityResult result = algorithm.run(graph);

    BM_ASSERT(result.planar);
    BM_ASSERT(result.embedding.has_value());
    BM_ASSERT(result.embedding->clockwiseEdgesAroundVertex.size() == 1);
}

BM_TEST(BoyerMyrvoldRunHandlesSimpleTreePlaceholder) {
    Graph graph(4);

    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 3);

    BoyerMyrvoldPlanarity algorithm;
    const PlanarityResult result = algorithm.run(graph);

    BM_ASSERT(result.planar);
    BM_ASSERT(result.embedding.has_value());
    BM_ASSERT(result.embedding->clockwiseEdgesAroundVertex.size() == 4);
    BM_ASSERT(!result.certificate.has_value());
}

BM_TEST(BoyerMyrvoldRunRejectsParallelEdgesBeforeAlgorithm) {
    Graph graph(3);

    graph.addEdge(0, 1);
    graph.addEdge(1, 0);

    BoyerMyrvoldPlanarity algorithm;

    bool threw = false;

    try {
        algorithm.run(graph);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    BM_ASSERT(threw);
}

BM_TEST(BoyerMyrvoldDetectsTriangleAsPlanar) {
    Graph graph(3);

    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 0);

    BoyerMyrvoldPlanarity algorithm;

    const PlanarityResult result = algorithm.run(graph);

    BM_ASSERT(result.planar);
}

BM_TEST(BoyerMyrvoldDetectsK33AsNonPlanar) {
    Graph graph(6);

    for (int left = 0; left < 3; ++left) {
        for (int right = 3; right < 6; ++right) {
            graph.addEdge(left, right);
        }
    }

    BoyerMyrvoldPlanarity algorithm;

    const PlanarityResult result = algorithm.run(graph);

    BM_ASSERT(!result.planar);
}

BM_TEST(BoyerMyrvoldDetectsK5AsNonPlanar) {
    Graph graph(5);

    for (int u = 0; u < 5; ++u) {
        for (int v = u + 1; v < 5; ++v) {
            graph.addEdge(u, v);
        }
    }

    BoyerMyrvoldPlanarity algorithm;

    const PlanarityResult result = algorithm.run(graph);

    BM_ASSERT(!result.planar);
}