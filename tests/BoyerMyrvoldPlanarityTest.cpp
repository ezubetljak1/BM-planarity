#include "TestSupport.hpp"

#include "bm/BoyerMyrvoldPlanarity.hpp"
#include "bm/Graph.hpp"

#include <stdexcept>
#include <vector>

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

BM_TEST(BoyerMyrvoldRunReturnsRecoveredEmbeddingForSimpleTree) {
    Graph graph(4);

    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 3);

    BoyerMyrvoldPlanarity algorithm;
    const PlanarityResult result = algorithm.run(graph);

    BM_ASSERT(result.planar);
    BM_ASSERT(result.embedding.has_value());
    BM_ASSERT(result.embedding->clockwiseEdgesAroundVertex.size() == 4);
    BM_ASSERT(result.embedding->clockwiseEdgesAroundVertex[0].size() == 1);
    BM_ASSERT(result.embedding->clockwiseEdgesAroundVertex[1].size() == 2);
    BM_ASSERT(result.embedding->clockwiseEdgesAroundVertex[2].size() == 2);
    BM_ASSERT(result.embedding->clockwiseEdgesAroundVertex[3].size() == 1);
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
BM_TEST(BoyerMyrvoldDetectsK4AsPlanar) {
    Graph graph(4);

    for (int u = 0; u < 4; ++u) {
        for (int v = u + 1; v < 4; ++v) {
            graph.addEdge(u, v);
        }
    }

    BoyerMyrvoldPlanarity algorithm;
    BM_ASSERT(algorithm.run(graph).planar);
}

BM_TEST(BoyerMyrvoldDetectsWheelAsPlanar) {
    Graph graph(6);

    for (int rim = 1; rim <= 5; ++rim) {
        graph.addEdge(0, rim);
        graph.addEdge(rim, rim == 5 ? 1 : rim + 1);
    }

    BoyerMyrvoldPlanarity algorithm;
    BM_ASSERT(algorithm.run(graph).planar);
}

BM_TEST(BoyerMyrvoldDetectsSubdividedK5AsNonPlanar) {
    Graph graph(6);

    graph.addEdge(0, 5);
    graph.addEdge(5, 1);

    for (int u = 0; u < 5; ++u) {
        for (int v = u + 1; v < 5; ++v) {
            if (u == 0 && v == 1) {
                continue;
            }

            graph.addEdge(u, v);
        }
    }

    BoyerMyrvoldPlanarity algorithm;
    BM_ASSERT(!algorithm.run(graph).planar);
}

BM_TEST(BoyerMyrvoldDetectsSubdividedK33AsNonPlanar) {
    Graph graph(7);

    graph.addEdge(0, 6);
    graph.addEdge(6, 3);

    for (int left = 0; left < 3; ++left) {
        for (int right = 3; right < 6; ++right) {
            if (left == 0 && right == 3) {
                continue;
            }

            graph.addEdge(left, right);
        }
    }

    BoyerMyrvoldPlanarity algorithm;
    BM_ASSERT(!algorithm.run(graph).planar);
}

BM_TEST(BoyerMyrvoldDetectsDisconnectedGraphWithNonPlanarComponent) {
    Graph graph(9);

    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 0);

    for (int left = 3; left < 6; ++left) {
        for (int right = 6; right < 9; ++right) {
            graph.addEdge(left, right);
        }
    }

    BoyerMyrvoldPlanarity algorithm;
    BM_ASSERT(!algorithm.run(graph).planar);
}

BM_TEST(BoyerMyrvoldDetectsArticulationGraphWithTwoK4BlocksAsPlanar) {
    Graph graph(7);

    const int firstBlock[] = {0, 1, 2, 3};
    const int secondBlock[] = {0, 4, 5, 6};

    for (const int* block : {firstBlock, secondBlock}) {
        for (int i = 0; i < 4; ++i) {
            for (int j = i + 1; j < 4; ++j) {
                graph.addEdge(block[i], block[j]);
            }
        }
    }

    BoyerMyrvoldPlanarity algorithm;
    BM_ASSERT(algorithm.run(graph).planar);
}

BM_TEST(BoyerMyrvoldExhaustivelyChecksAllFiveVertexGraphs) {
    constexpr int vertexCount = 5;

    struct Pair {
        int u;
        int v;
    };

    std::vector<Pair> possibleEdges;

    for (int u = 0; u < vertexCount; ++u) {
        for (int v = u + 1; v < vertexCount; ++v) {
            possibleEdges.push_back({u, v});
        }
    }

    const int graphCount = 1 << static_cast<int>(possibleEdges.size());

    BoyerMyrvoldPlanarity algorithm;

    for (int mask = 0; mask < graphCount; ++mask) {
        Graph graph(vertexCount);

        for (int edgeIndex = 0; edgeIndex < static_cast<int>(possibleEdges.size()); ++edgeIndex) {
            if ((mask & (1 << edgeIndex)) != 0) {
                graph.addEdge(possibleEdges[edgeIndex].u, possibleEdges[edgeIndex].v);
            }
        }

        const bool expectedPlanar = mask != graphCount - 1;

        BM_ASSERT(algorithm.run(graph).planar == expectedPlanar);
    }
}
