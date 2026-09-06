#include "bm/BmEmbeddingState.hpp"
#include "bm/BmKuratowskiFailureFactory.hpp"
#include "bm/BmKuratowskiIsolationPreparation.hpp"
#include "bm/BmWalkdown.hpp"
#include "bm/BmWalkup.hpp"
#include "bm/DfsPreprocessor.hpp"
#include "bm/Graph.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using namespace bm;

const char* minorName(BmKuratowskiMinorType type) {
    switch (type) {
    case BmKuratowskiMinorType::A:
        return "A";
    case BmKuratowskiMinorType::B:
        return "B";
    case BmKuratowskiMinorType::C:
        return "C";
    case BmKuratowskiMinorType::D:
        return "D";
    case BmKuratowskiMinorType::E:
        return "E";
    default:
        return "UNKNOWN";
    }
}

template <typename Consumer>
bool inspectFirstFailure(const Graph& graph, Consumer&& consume) {
    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);
    BmEmbeddingState state(graph, dfsInfo);
    BmWalkup walkup;
    BmWalkdown walkdown;

    for (int dfi = dfsInfo.vertexCount - 1; dfi >= 0; --dfi) {
        const int vertex = dfsInfo.vertexAtDfsIndex[dfi];

        for (int child : dfsInfo.children[vertex]) {
            state.createTreeEdgeBicomp(vertex, child);
        }

        for (int backEdgeIndex :
             dfsInfo.backEdgeIndicesFromAncestor[vertex]) {
            const DfsBackEdge& backEdge =
                dfsInfo.backEdges[backEdgeIndex];
            walkup.run(state, vertex, backEdge.descendant, backEdge.edgeId);
        }

        for (int child : dfsInfo.children[vertex]) {
            if (!state.hasPertinentRoots(child)) {
                continue;
            }

            const BmWalkdownResult result =
                walkdown.run(state, vertex, state.rootForChild(child));

            if (!result.completed) {
                if (!result.failure.has_value()) {
                    throw std::logic_error("Walkdown failure has no extraction context.");
                }

                consume(state, *result.failure);
                return true;
            }
        }

        for (int backEdgeIndex :
             dfsInfo.backEdgeIndicesFromAncestor[vertex]) {
            const DfsBackEdge& backEdge =
                dfsInfo.backEdges[backEdgeIndex];

            if (!state.isOriginalEdgeEmbedded(backEdge.edgeId)) {
                consume(
                    state,
                    BmKuratowskiFailureFactory::fromUnembeddedBackedge(
                        state,
                        vertex,
                        backEdge
                    )
                );
                return true;
            }
        }
    }

    return false;
}

void classifyOneGraph(const Graph& graph) {
    const bool nonPlanar = inspectFirstFailure(
        graph,
        [](const BmEmbeddingState& state, const BmWalkdownFailure& failure) {
            const BmPreparedKuratowskiIsolation prepared =
                BmKuratowskiIsolationPreparation::prepare(state, failure);

            std::cout << "NONPLANAR " << minorName(prepared.minorType) << '\n';
        }
    );

    if (!nonPlanar) {
        std::cout << "PLANAR\n";
    }
}

} // namespace

int main() {
    try {
        int graphCount = 0;

        if (!(std::cin >> graphCount) || graphCount < 0) {
            throw std::invalid_argument("Expected a non-negative graph count.");
        }

        for (int graphIndex = 0; graphIndex < graphCount; ++graphIndex) {
            int vertexCount = 0;
            int edgeCount = 0;

            if (!(std::cin >> vertexCount >> edgeCount)) {
                throw std::invalid_argument("Expected graph vertex and edge counts.");
            }

            Graph graph(vertexCount);

            for (int edgeIndex = 0; edgeIndex < edgeCount; ++edgeIndex) {
                int first = -1;
                int second = -1;

                if (!(std::cin >> first >> second)) {
                    throw std::invalid_argument("Expected an edge endpoint pair.");
                }

                graph.addEdge(first, second);
            }

            classifyOneGraph(graph);
        }
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
