#include "TestSupport.hpp"

#include "bm/BmPartialEmbedding.hpp"
#include "bm/BmExternalFaceTraversal.hpp"

using namespace bm;

BM_TEST(BmPartialEmbeddingCreatesOriginalInternalVertices) {
    BmPartialEmbedding embedding(3);

    BM_ASSERT(embedding.internalVertexCount() == 3);
    BM_ASSERT(embedding.originalInternalVertex(0) == 0);
    BM_ASSERT(embedding.originalInternalVertex(1) == 1);
    BM_ASSERT(embedding.originalInternalVertex(2) == 2);

    BM_ASSERT(embedding.internalVertex(0).kind == BmInternalVertexKind::Original);
}

BM_TEST(BmPartialEmbeddingCreatesTreeEdgeBicomp) {
    BmPartialEmbedding embedding(2);

    const BmTreeBicompEmbedding tree = embedding.createTreeEdgeBicomp(0, 0, 1, 7);

    BM_ASSERT(embedding.internalVertexCount() == 3);
    BM_ASSERT(embedding.embeddedEdgeCount() == 1);
    BM_ASSERT(embedding.halfEdgeCount() == 2);

    const auto& rootVertex = embedding.internalVertex(tree.rootInternalVertexId);
    const auto& childVertex = embedding.internalVertex(tree.childInternalVertexId);

    BM_ASSERT(rootVertex.kind == BmInternalVertexKind::BicompRoot);
    BM_ASSERT(rootVertex.originalVertex == 0);
    BM_ASSERT(rootVertex.bicompRootId == 0);

    BM_ASSERT(childVertex.kind == BmInternalVertexKind::Original);
    BM_ASSERT(childVertex.originalVertex == 1);

    const auto& embeddedEdge = embedding.embeddedEdge(tree.embeddedEdgeId);

    BM_ASSERT(embeddedEdge.originalEdgeId == 7);
    BM_ASSERT(embeddedEdge.halfEdgeA == tree.rootToChildHalfEdgeId);
    BM_ASSERT(embeddedEdge.halfEdgeB == tree.childToRootHalfEdgeId);
}



BM_TEST(BmPartialEmbeddingProvidesRootLookupHelpers) {
    BmPartialEmbedding embedding(2);

    const BmTreeBicompEmbedding tree = embedding.createTreeEdgeBicomp(3, 0, 1, 7);

    BM_ASSERT(embedding.isBicompRootVertex(tree.rootInternalVertexId));
    BM_ASSERT(!embedding.isBicompRootVertex(tree.childInternalVertexId));

    BM_ASSERT(embedding.bicompRootIdForInternalVertex(tree.rootInternalVertexId) == 3);

    BM_ASSERT(embedding.originalVertexForInternalVertex(tree.rootInternalVertexId) == 0);
    BM_ASSERT(embedding.originalVertexForInternalVertex(tree.childInternalVertexId) == 1);
}

BM_TEST(BmPartialEmbeddingAddsGeneralEmbeddedEdge) {
    BmPartialEmbedding embedding(3);

    const int vertex0 = embedding.originalInternalVertex(0);
    const int vertex1 = embedding.originalInternalVertex(1);

    const int edgeId = embedding.addEmbeddedEdge(vertex0, vertex1, 7);

    const BmEmbeddedEdge& edge = embedding.embeddedEdge(edgeId);

    BM_ASSERT(edge.originalEdgeId == 7);
    BM_ASSERT(edge.sign == 1);

    BM_ASSERT(!embedding.adjacencyEmpty(vertex0));
    BM_ASSERT(!embedding.adjacencyEmpty(vertex1));

    BM_ASSERT(embedding.halfEdge(edge.halfEdgeA).from == vertex0);
    BM_ASSERT(embedding.halfEdge(edge.halfEdgeA).to == vertex1);

    BM_ASSERT(embedding.halfEdge(edge.halfEdgeB).from == vertex1);
    BM_ASSERT(embedding.halfEdge(edge.halfEdgeB).to == vertex0);
}

BM_TEST(BmPartialEmbeddingMaintainsCircularAdjacencyList) {
    BmPartialEmbedding embedding(3);

    const int vertex0 = embedding.originalInternalVertex(0);
    const int vertex1 = embedding.originalInternalVertex(1);
    const int vertex2 = embedding.originalInternalVertex(2);

    const int edge01 = embedding.addEmbeddedEdge(vertex0, vertex1, 0);

    const int edge02 = embedding.addEmbeddedEdge(vertex0, vertex2, 1);

    const int halfEdge01 = embedding.embeddedEdge(edge01).halfEdgeA;
    const int halfEdge02 = embedding.embeddedEdge(edge02).halfEdgeA;

    BM_ASSERT(embedding.nextAroundVertex(halfEdge01) == halfEdge02);
    BM_ASSERT(embedding.nextAroundVertex(halfEdge02) == halfEdge01);

    BM_ASSERT(embedding.previousAroundVertex(halfEdge01) == halfEdge02);
    BM_ASSERT(embedding.previousAroundVertex(halfEdge02) == halfEdge01);
}

BM_TEST(BmPartialEmbeddingRedirectsVirtualRootAdjacency) {
    BmPartialEmbedding embedding(2);

    const BmTreeBicompEmbedding tree = embedding.createTreeEdgeBicomp(0, 0, 1, 7);

    const int realRoot = embedding.originalInternalVertex(0);

    embedding.redirectAdjacencyToVertex(tree.rootInternalVertexId, realRoot);

    BM_ASSERT(embedding.halfEdge(tree.rootToChildHalfEdgeId).from == realRoot);

    BM_ASSERT(embedding.halfEdge(tree.childToRootHalfEdgeId).to == realRoot);
}

BM_TEST(BmPartialEmbeddingReversesAdjacencyOrientation) {
    BmPartialEmbedding embedding(3);

    const int vertex0 = embedding.originalInternalVertex(0);
    const int vertex1 = embedding.originalInternalVertex(1);
    const int vertex2 = embedding.originalInternalVertex(2);

    const int edge01 = embedding.addEmbeddedEdge(vertex0, vertex1, 0);

    const int edge02 = embedding.addEmbeddedEdge(vertex0, vertex2, 1);

    const int halfEdge01 = embedding.embeddedEdge(edge01).halfEdgeA;
    const int halfEdge02 = embedding.embeddedEdge(edge02).halfEdgeA;

    embedding.setExternalFaceHalfEdges(vertex0, halfEdge01, halfEdge02);

    embedding.reverseAdjacencyOrientation(vertex0);

    BM_ASSERT(embedding.externalFaceHalfEdge(vertex0, 0) == halfEdge02);
    BM_ASSERT(embedding.externalFaceHalfEdge(vertex0, 1) == halfEdge01);
}
BM_TEST(BmPartialEmbeddingShortcutsExternalFacePathWithoutCreatingFakeEdge) {
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

    const int edgeCountBefore = embedding.embeddedEdgeCount();

    embedding.shortcutExternalFacePath(vertex0, 0, vertex2, 0);

    BM_ASSERT(embedding.embeddedEdgeCount() == edgeCountBefore);
    BM_ASSERT(embedding.externalFaceNeighbor(vertex0, 0) == vertex2);
    BM_ASSERT(embedding.externalFaceNeighbor(vertex2, 0) == vertex0);

    BmExternalFaceTraversal traversal(embedding);

    BmExternalFacePosition start;
    start.internalVertexId = vertex0;
    start.linkIndex = 1;

    const BmExternalFacePosition afterShortcut = traversal.successor(start);

    BM_ASSERT(afterShortcut.internalVertexId == vertex2);
    BM_ASSERT(afterShortcut.linkIndex == 0);
}
