#include "TestSupport.hpp"

#include "bm/BmEmbeddingState.hpp"
#include "bm/BmKuratowskiConnectionFinder.hpp"
#include "bm/BmKuratowskiExtractionContext.hpp"
#include "bm/BmKuratowskiFailureFactory.hpp"
#include "bm/BmKuratowskiMinorClassifier.hpp"
#include "bm/BmKuratowskiPathMarker.hpp"
#include "bm/BmKuratowskiExtractor.hpp"
#include "bm/KuratowskiCertificateVerifier.hpp"
#include "bm/BmWalkdown.hpp"
#include "bm/BmWalkup.hpp"
#include "bm/DfsPreprocessor.hpp"
#include "bm/Graph.hpp"

using namespace bm;

namespace {


Graph makeK33() {
    Graph graph(6);

    for (int left = 0; left < 3; ++left) {
        for (int right = 3; right < 6; ++right) {
            graph.addEdge(left, right);
        }
    }

    return graph;
}

Graph makeK5() {
    Graph graph(5);

    for (int first = 0; first < 5; ++first) {
        for (int second = first + 1; second < 5; ++second) {
            graph.addEdge(first, second);
        }
    }

    return graph;
}

Graph makeInitialMinorBGraph() {
    Graph graph(6);

    graph.addEdge(0, 1);
    graph.addEdge(0, 2);
    graph.addEdge(0, 3);
    graph.addEdge(0, 4);
    graph.addEdge(0, 5);
    graph.addEdge(1, 3);
    graph.addEdge(1, 4);
    graph.addEdge(1, 5);
    graph.addEdge(2, 3);
    graph.addEdge(2, 4);
    graph.addEdge(2, 5);
    graph.addEdge(3, 5);

    return graph;
}

template <typename Inspector>
void inspectFirstFailure(const Graph& graph, Inspector&& inspector) {
    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);
    BmEmbeddingState state(graph, dfsInfo);
    BmWalkup walkup;
    BmWalkdown walkdown;

    for (int dfi = dfsInfo.vertexCount - 1; dfi >= 0; --dfi) {
        const int vertex = dfsInfo.vertexAtDfsIndex[static_cast<std::size_t>(dfi)];

        for (int child : dfsInfo.children[static_cast<std::size_t>(vertex)]) {
            state.createTreeEdgeBicomp(vertex, child);
        }

        for (int backEdgeIndex :
             dfsInfo.backEdgeIndicesFromAncestor[static_cast<std::size_t>(vertex)]) {
            const DfsBackEdge& backEdge =
                dfsInfo.backEdges[static_cast<std::size_t>(backEdgeIndex)];
            walkup.run(state, vertex, backEdge.descendant, backEdge.edgeId);
        }

        for (int child : dfsInfo.children[static_cast<std::size_t>(vertex)]) {
            if (!state.hasPertinentRoots(child)) {
                continue;
            }

            const BmWalkdownResult result = walkdown.run(state, vertex, state.rootForChild(child));

            if (!result.completed) {
                BM_ASSERT(result.failure.has_value());
                inspector(state, *result.failure);
                return;
            }
        }

        for (int backEdgeIndex :
             dfsInfo.backEdgeIndicesFromAncestor[static_cast<std::size_t>(vertex)]) {
            const DfsBackEdge& backEdge =
                dfsInfo.backEdges[static_cast<std::size_t>(backEdgeIndex)];

            if (!state.isOriginalEdgeEmbedded(backEdge.edgeId)) {
                inspector(
                    state,
                    BmKuratowskiFailureFactory::fromUnembeddedBackedge(
                        state,
                        vertex,
                        backEdge
                    )
                );
                return;
            }
        }
    }

    throw std::logic_error("Expected graph to produce a non-planarity failure.");
}

} // namespace

BM_TEST(DfsPreprocessorBuildsContiguousSubtreeIntervals) {
    Graph graph(6);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(1, 3);
    graph.addEdge(0, 4);
    graph.addEdge(4, 5);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    BM_ASSERT(BmKuratowskiConnectionFinder::isInSubtree(dfsInfo, 1, 1));
    BM_ASSERT(BmKuratowskiConnectionFinder::isInSubtree(dfsInfo, 1, 2));
    BM_ASSERT(BmKuratowskiConnectionFinder::isInSubtree(dfsInfo, 1, 3));
    BM_ASSERT(!BmKuratowskiConnectionFinder::isInSubtree(dfsInfo, 1, 4));
    BM_ASSERT(!BmKuratowskiConnectionFinder::isInSubtree(dfsInfo, 1, 5));
}

BM_TEST(BmKuratowskiFailureFactoryBuildsContextForUnembeddedK5Backedge) {
    const Graph graph = makeK5();

    inspectFirstFailure(graph, [](const BmEmbeddingState& state, const BmWalkdownFailure& failure) {
        BM_ASSERT(failure.reason == BmWalkdownFailureReason::UnembeddedBackedge);
        BM_ASSERT(failure.unembeddedEdgeId >= 0);
        BM_ASSERT(failure.unembeddedDescendantVertex >= 0);

        const BmKuratowskiExtractionContext context =
            BmKuratowskiExtractionContextBuilder::initialize(state, failure);

        BM_ASSERT(context.currentVertex == failure.currentVertex);
        BM_ASSERT(context.centralRootId == failure.topRootId);
        BM_ASSERT(context.pertinentVertex >= 0);
    });
}

BM_TEST(BmKuratowskiMinorClassifierRecognizesInitialMinorAForK33) {
    const Graph graph = makeK33();

    inspectFirstFailure(graph, [](const BmEmbeddingState& state, const BmWalkdownFailure& failure) {
        const BmKuratowskiExtractionContext context =
            BmKuratowskiExtractionContextBuilder::initialize(state, failure);

        BM_ASSERT(
            BmKuratowskiMinorClassifier::classifyInitial(state, context)
            == BmKuratowskiMinorType::A
        );
    });
}

BM_TEST(BmKuratowskiPathMarkerMarksDfsAndRealExternalFacePaths) {
    Graph graph(4);
    const int edge01 = graph.addEdge(0, 1);
    const int edge12 = graph.addEdge(1, 2);
    const int edge23 = graph.addEdge(2, 3);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);
    BmEmbeddingState state(graph, dfsInfo);

    state.createTreeEdgeBicomp(0, 1);
    state.createTreeEdgeBicomp(1, 2);
    state.createTreeEdgeBicomp(2, 3);

    BmKuratowskiPathMarker marker(state);
    marker.markDfsPath(0, 3);

    BM_ASSERT(marker.isOriginalEdgeMarked(edge01));
    BM_ASSERT(marker.isOriginalEdgeMarked(edge12));
    BM_ASSERT(marker.isOriginalEdgeMarked(edge23));
}

BM_TEST(BmKuratowskiConnectionFinderFindsAncestorAndCurrentConnectionsForK33) {
    const Graph graph = makeK33();

    inspectFirstFailure(graph, [](const BmEmbeddingState& state, const BmWalkdownFailure& failure) {
        const BmKuratowskiExtractionContext context =
            BmKuratowskiExtractionContextBuilder::initialize(state, failure);

        const int x = state.originalVertexForInternalVertex(context.x.internalVertexId);
        const auto xConnection = BmKuratowskiConnectionFinder::findToAncestor(
            state,
            context.currentVertex,
            x
        );
        const auto wConnection = BmKuratowskiConnectionFinder::findToCurrentVertex(
            state,
            context.currentVertex,
            context.pertinentVertex
        );

        BM_ASSERT(xConnection.has_value());
        BM_ASSERT(wConnection.has_value());
        BM_ASSERT(wConnection->ancestorVertex == context.currentVertex);
    });
}

BM_TEST(BmKuratowskiExtractorIsolatesInitialMinorAForK33) {
    const Graph graph = makeK33();

    inspectFirstFailure(graph, [&](const BmEmbeddingState& state, const BmWalkdownFailure& failure) {
        const KuratowskiCertificate certificate =
            BmKuratowskiExtractor::extractInitialMinor(state, failure);

        KuratowskiCertificateVerifier::validate(graph, certificate);
        BM_ASSERT(certificate.type == KuratowskiType::K33);
    });
}


BM_TEST(BmKuratowskiMinorClassifierRecognizesInitialMinorB) {
    const Graph graph = makeInitialMinorBGraph();

    inspectFirstFailure(graph, [](const BmEmbeddingState& state, const BmWalkdownFailure& failure) {
        const BmKuratowskiExtractionContext context =
            BmKuratowskiExtractionContextBuilder::initialize(state, failure);

        BM_ASSERT(
            BmKuratowskiMinorClassifier::classifyInitial(state, context)
            == BmKuratowskiMinorType::B
        );
    });
}

BM_TEST(BmKuratowskiExtractorIsolatesInitialMinorB) {
    const Graph graph = makeInitialMinorBGraph();

    inspectFirstFailure(graph, [&](const BmEmbeddingState& state, const BmWalkdownFailure& failure) {
        const KuratowskiCertificate certificate =
            BmKuratowskiExtractor::extractInitialMinor(state, failure);

        KuratowskiCertificateVerifier::validate(graph, certificate);
        BM_ASSERT(certificate.type == KuratowskiType::K33);
    });
}
