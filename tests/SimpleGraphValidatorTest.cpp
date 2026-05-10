#include "TestSupport.hpp"

#include "bm/Graph.hpp"
#include "bm/SimpleGraphValidator.hpp"

#include <stdexcept>

using namespace bm;

BM_TEST(SimpleGraphValidatorAcceptsEmptyGraph) {
    Graph graph(0);
    
    SimpleGraphValidator::validate(graph);

    BM_ASSERT(graph.vertexCount() == 0);
}

BM_TEST(SimpleGraphValidatorAcceptsSimpleGraph) {
    Graph graph(4);

    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 3);
    graph.addEdge(3, 0);

    SimpleGraphValidator::validate(graph);

    BM_ASSERT(graph.edgeCount() == 4);
}

BM_TEST(SimpleGraphValidatorRejectsParallelEdges) {
    Graph graph(3);

    graph.addEdge(0, 1);
    graph.addEdge(1, 0);

    bool threw = false;

    try {
        SimpleGraphValidator::validate(graph);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    BM_ASSERT(threw);
}