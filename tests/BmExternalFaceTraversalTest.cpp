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

    const BmExternalFaceStep firstStep = traversal.step(start);

    BM_ASSERT(firstStep.fromInternalVertexId == tree.rootInternalVertexId);
    BM_ASSERT(firstStep.toInternalVertexId == tree.childInternalVertexId);
    BM_ASSERT(firstStep.usedHalfEdgeId == tree.rootToChildHalfEdgeId);

    BM_ASSERT(firstStep.successor.internalVertexId == tree.childInternalVertexId);
    BM_ASSERT(firstStep.successor.linkIndex == 0);
}

BM_TEST(BmExternalFaceTraversalCollectsTreeBicompCycle) {
    BmPartialEmbedding embedding(2);

    const BmTreeBicompEmbedding tree = embedding.createTreeEdgeBicomp(0, 0, 1, 7);

    BmExternalFaceTraversal traversal(embedding);

    BmExternalFacePosition start;
    start.internalVertexId = tree.rootInternalVertexId;
    start.linkIndex = 0;

    const auto cycle = traversal.collectCycle(start, 10);

    BM_ASSERT(cycle.size() == 2);

    BM_ASSERT(cycle[0].fromInternalVertexId == tree.rootInternalVertexId);
    BM_ASSERT(cycle[0].toInternalVertexId == tree.childInternalVertexId);

    BM_ASSERT(cycle[1].fromInternalVertexId == tree.childInternalVertexId);
    BM_ASSERT(cycle[1].toInternalVertexId == tree.rootInternalVertexId);
}