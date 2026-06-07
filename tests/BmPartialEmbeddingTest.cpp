#include "TestSupport.hpp"

#include "bm/BmPartialEmbedding.hpp"

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

BM_TEST(BmPartialEmbeddingLinksTreeBicompExternalFace) {
    BmPartialEmbedding embedding(2);

    const BmTreeBicompEmbedding tree = embedding.createTreeEdgeBicomp(0, 0, 1, 7);

    const auto& rootToChild = embedding.halfEdge(tree.rootToChildHalfEdgeId);
    const auto& childToRoot = embedding.halfEdge(tree.childToRootHalfEdgeId);

    BM_ASSERT(rootToChild.from == tree.rootInternalVertexId);
    BM_ASSERT(rootToChild.to == tree.childInternalVertexId);
    BM_ASSERT(rootToChild.twin == tree.childToRootHalfEdgeId);

    BM_ASSERT(childToRoot.from == tree.childInternalVertexId);
    BM_ASSERT(childToRoot.to == tree.rootInternalVertexId);
    BM_ASSERT(childToRoot.twin == tree.rootToChildHalfEdgeId);

    BM_ASSERT(rootToChild.nextOnExternalFace == tree.childToRootHalfEdgeId);
    BM_ASSERT(childToRoot.nextOnExternalFace == tree.rootToChildHalfEdgeId);

    const auto face = embedding.externalFaceVertices(tree.rootToChildHalfEdgeId, 4);

    BM_ASSERT(face.size() == 2);
    BM_ASSERT(face[0] == tree.rootInternalVertexId);
    BM_ASSERT(face[1] == tree.childInternalVertexId);
}

BM_TEST(BmPartialEmbeddingProvidesExternalFaceHelpers) {
    BmPartialEmbedding embedding(2);

    const BmTreeBicompEmbedding tree = embedding.createTreeEdgeBicomp(0, 0, 1, 7);

    BM_ASSERT(embedding.twinHalfEdge(tree.rootToChildHalfEdgeId) == tree.childToRootHalfEdgeId);

    BM_ASSERT(embedding.twinHalfEdge(tree.childToRootHalfEdgeId) == tree.rootToChildHalfEdgeId);

    BM_ASSERT(embedding.nextOnExternalFace(tree.rootToChildHalfEdgeId) ==
              tree.childToRootHalfEdgeId);

    BM_ASSERT(embedding.previousOnExternalFace(tree.rootToChildHalfEdgeId) ==
              tree.childToRootHalfEdgeId);

    BM_ASSERT(embedding.externalFaceHalfEdge(tree.rootInternalVertexId, 0) ==
              tree.rootToChildHalfEdgeId);

    BM_ASSERT(embedding.externalFaceHalfEdge(tree.childInternalVertexId, 0) ==
              tree.childToRootHalfEdgeId);
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

    const int edgeId = embedding.addEmbeddedEdge(vertex0, vertex1, 7, false);

    const BmEmbeddedEdge& edge = embedding.embeddedEdge(edgeId);

    BM_ASSERT(edge.originalEdgeId == 7);
    BM_ASSERT(!edge.shortCircuit);
    BM_ASSERT(edge.sign == 1);
    BM_ASSERT(edge.active);

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

    const int edge01 = embedding.addEmbeddedEdge(vertex0, vertex1, 0, false);

    const int edge02 = embedding.addEmbeddedEdge(vertex0, vertex2, 1, false);

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

    const int edge01 = embedding.addEmbeddedEdge(vertex0, vertex1, 0, false);

    const int edge02 = embedding.addEmbeddedEdge(vertex0, vertex2, 1, false);

    const int halfEdge01 = embedding.embeddedEdge(edge01).halfEdgeA;
    const int halfEdge02 = embedding.embeddedEdge(edge02).halfEdgeA;

    embedding.setExternalFaceHalfEdges(vertex0, halfEdge01, halfEdge02);

    embedding.reverseAdjacencyOrientation(vertex0);

    BM_ASSERT(embedding.externalFaceHalfEdge(vertex0, 0) == halfEdge02);
    BM_ASSERT(embedding.externalFaceHalfEdge(vertex0, 1) == halfEdge01);
}