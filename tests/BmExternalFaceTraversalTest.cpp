#include "TestSupport.hpp"

#include "bm/BmExternalFaceTraversal.hpp"
#include "bm/BmPartialEmbedding.hpp"

using namespace bm;

BM_TEST(BmExternalFaceTraversalStepsAcrossTreeBicomp) {
    BmPartialEmbedding embedding(2);

    const BmTreeBicompEmbedding tree = embedding.createTreeEdgeBicomp(0, 0, 1, 7);

    BmExternalFaceTraversal traversal(embedding);

    BmExternalFacePosition start;
    start.internalVertexId = tree.rootInternalVertexId;
    start.linkIndex = 0;

    const BmExternalFacePosition successor = traversal.successor(start);

    BM_ASSERT(successor.internalVertexId == tree.childInternalVertexId);
    BM_ASSERT(successor.linkIndex == 0);
}

BM_TEST(BmExternalFaceTraversalReturnsToTreeBicompRoot) {
    BmPartialEmbedding embedding(2);

    const BmTreeBicompEmbedding tree = embedding.createTreeEdgeBicomp(0, 0, 1, 7);

    BmExternalFaceTraversal traversal(embedding);

    BmExternalFacePosition start;
    start.internalVertexId = tree.rootInternalVertexId;
    start.linkIndex = 0;

    const BmExternalFacePosition child = traversal.successor(start);
    const BmExternalFacePosition root = traversal.successor(child);

    BM_ASSERT(root.internalVertexId == start.internalVertexId);
    BM_ASSERT(root.linkIndex == start.linkIndex);
}
