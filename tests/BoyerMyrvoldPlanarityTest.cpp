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