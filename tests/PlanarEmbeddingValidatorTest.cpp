#include "TestSupport.hpp"

#include "bm/BoyerMyrvoldPlanarity.hpp"
#include "bm/Graph.hpp"
#include "bm/PlanarEmbeddingValidator.hpp"

#include <stdexcept>

using namespace bm;

BM_TEST(PlanarEmbeddingValidatorAcceptsRecoveredK4Embedding) {
    Graph graph(4);

    for (int u = 0; u < 4; ++u) {
        for (int v = u + 1; v < 4; ++v) {
            graph.addEdge(u, v);
        }
    }

    BoyerMyrvoldPlanarity algorithm;
    const PlanarityResult result = algorithm.run(graph);

    BM_ASSERT(result.planar);
    BM_ASSERT(result.embedding.has_value());

    PlanarEmbeddingValidator::validate(graph, *result.embedding);
}

BM_TEST(PlanarEmbeddingValidatorAcceptsRecoveredArticulationEmbedding) {
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
    const PlanarityResult result = algorithm.run(graph);

    BM_ASSERT(result.planar);
    BM_ASSERT(result.embedding.has_value());

    PlanarEmbeddingValidator::validate(graph, *result.embedding);
}

BM_TEST(PlanarEmbeddingValidatorRejectsMissingEdgeOccurrence) {
    Graph graph(3);
    const int edge01 = graph.addEdge(0, 1);
    const int edge12 = graph.addEdge(1, 2);
    const int edge20 = graph.addEdge(2, 0);

    PlanarEmbedding embedding;
    embedding.clockwiseEdgesAroundVertex = {
        {edge01, edge20},
        {edge01, edge12},
        {edge12}
    };

    bool threw = false;

    try {
        PlanarEmbeddingValidator::validate(graph, embedding);
    } catch (const std::logic_error&) {
        threw = true;
    }

    BM_ASSERT(threw);
}

BM_TEST(PlanarEmbeddingValidatorRejectsNonPlanarRotationSystem) {
    Graph graph(4);

    const int edge01 = graph.addEdge(0, 1);
    const int edge02 = graph.addEdge(0, 2);
    const int edge03 = graph.addEdge(0, 3);
    const int edge12 = graph.addEdge(1, 2);
    const int edge13 = graph.addEdge(1, 3);
    const int edge23 = graph.addEdge(2, 3);

    // This rotation system represents a positive-genus embedding of K4,
    // not a planar embedding.
    PlanarEmbedding embedding;
    embedding.clockwiseEdgesAroundVertex = {
        {edge01, edge02, edge03},
        {edge01, edge12, edge13},
        {edge02, edge12, edge23},
        {edge03, edge13, edge23}
    };

    bool threw = false;

    try {
        PlanarEmbeddingValidator::validate(graph, embedding);
    } catch (const std::logic_error&) {
        threw = true;
    }

    BM_ASSERT(threw);
}
