#include "TestSupport.hpp"

#include "bm/BmEmbeddingState.hpp"
#include "bm/BmKuratowskiExtractionContext.hpp"
#include "bm/BmRealExternalFaceTraversal.hpp"
#include "bm/BmWalkdown.hpp"
#include "bm/BmWalkup.hpp"
#include "bm/DfsPreprocessor.hpp"
#include "bm/Graph.hpp"

#include <optional>

using namespace bm;

namespace {

template <typename Inspector>
void inspectFirstK33Failure(const Graph& graph, Inspector&& inspector) {
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

            const BmWalkdownResult result = walkdown.run(state, vertex, state.rootForChild(child));

            if (!result.completed) {
                BM_ASSERT(result.failure.has_value());
                inspector(state, *result.failure);
                return;
            }
        }
    }

    throw std::logic_error("Expected K3,3 to produce a Walkdown failure.");
}

Graph makeK33() {
    Graph graph(6);

    for (int left = 0; left < 3; ++left) {
        for (int right = 3; right < 6; ++right) {
            graph.addEdge(left, right);
        }
    }

    return graph;
}

} // namespace

BM_TEST(BmRealExternalFaceTraversalIgnoresOptimizationShortcut) {
    BmPartialEmbedding embedding(4);

    const int vertex0 = embedding.originalInternalVertex(0);
    const int vertex1 = embedding.originalInternalVertex(1);
    const int vertex2 = embedding.originalInternalVertex(2);
    const int vertex3 = embedding.originalInternalVertex(3);

    const int edge01 = embedding.addEmbeddedEdge(vertex0, vertex1, 0);
    const int edge12 = embedding.addEmbeddedEdge(vertex1, vertex2, 1);
    const int edge23 = embedding.addEmbeddedEdge(vertex2, vertex3, 2);
    const int edge30 = embedding.addEmbeddedEdge(vertex3, vertex0, 3);

    embedding.setExternalFaceHalfEdges(vertex0, embedding.embeddedEdge(edge01).halfEdgeA,
                                       embedding.embeddedEdge(edge30).halfEdgeB);
    embedding.setExternalFaceHalfEdges(vertex1, embedding.embeddedEdge(edge01).halfEdgeB,
                                       embedding.embeddedEdge(edge12).halfEdgeA);
    embedding.setExternalFaceHalfEdges(vertex2, embedding.embeddedEdge(edge12).halfEdgeB,
                                       embedding.embeddedEdge(edge23).halfEdgeA);
    embedding.setExternalFaceHalfEdges(vertex3, embedding.embeddedEdge(edge23).halfEdgeB,
                                       embedding.embeddedEdge(edge30).halfEdgeA);

    embedding.setExternalFaceNeighbors(vertex0, vertex1, vertex3);
    embedding.setExternalFaceNeighbors(vertex1, vertex0, vertex2);
    embedding.setExternalFaceNeighbors(vertex2, vertex1, vertex3);
    embedding.setExternalFaceNeighbors(vertex3, vertex2, vertex0);

    embedding.shortcutExternalFacePath(vertex0, 0, vertex2, 0);

    BmExternalFaceTraversal optimizedTraversal(embedding);
    BmRealExternalFaceTraversal realTraversal(embedding);

    const BmExternalFacePosition optimized = optimizedTraversal.successor({vertex0, 1});
    const BmRealExternalFacePosition real = realTraversal.successor({vertex0, 1});

    BM_ASSERT(optimized.internalVertexId == vertex2);
    BM_ASSERT(real.internalVertexId == vertex1);
}

BM_TEST(BmWalkdownFailurePreservesIsolationContext) {
    const Graph graph = makeK33();
    inspectFirstK33Failure(graph, [](const BmEmbeddingState&, const BmWalkdownFailure& failure) {
        BM_ASSERT(failure.currentVertex >= 0);
        BM_ASSERT(failure.topRootId >= 0);
        BM_ASSERT(failure.rootOutgoingLink == 0 || failure.rootOutgoingLink == 1);
        BM_ASSERT(!failure.mergeStack.empty() || failure.blockingChildRootId >= 0);
    });
}

BM_TEST(BmKuratowskiExtractionContextInitializesFromK33Failure) {
    const Graph graph = makeK33();
    inspectFirstK33Failure(graph, [](const BmEmbeddingState& state, const BmWalkdownFailure& failure) {
        const BmKuratowskiExtractionContext context =
            BmKuratowskiExtractionContextBuilder::initialize(state, failure);

        BM_ASSERT(context.currentVertex == failure.currentVertex);
        BM_ASSERT(context.centralRootId >= 0);
        BM_ASSERT(context.centralRootInternalVertexId >= 0);
        BM_ASSERT(context.x.internalVertexId >= 0);
        BM_ASSERT(context.y.internalVertexId >= 0);
        BM_ASSERT(context.pertinentVertex >= 0);
    });
}
