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