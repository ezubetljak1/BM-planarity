#pragma once

#include <vector>

#include "bm/BmEmbeddingState.hpp"
#include "bm/BmExternalFaceTraversal.hpp"

namespace bm {

struct BmMergeFrame {
    int cutVertex = -1;
    int cutVertexIncomingLink = -1;

    int rootId = -1;
    int rootOutgoingLink = -1;
};

class BmEmbeddingOperations {
public:
    static void mergeTopBiconnectedComponent(BmEmbeddingState& state,
                                             std::vector<BmMergeFrame>& mergeStack);

    static void mergeAllBiconnectedComponents(BmEmbeddingState& state,
                                              std::vector<BmMergeFrame>& mergeStack);

    static BmExternalFacePosition getActiveSuccessorOnExternalFace(const BmEmbeddingState& state,
                                                                   BmExternalFacePosition start,
                                                                   int currentVertex);

    static int embedBackEdge(BmEmbeddingState& state, int rootId, int rootOutgoingLink,
                             int descendantVertex, int descendantIncomingLink, int currentVertex);

    static int embedShortCircuitEdge(BmEmbeddingState& state, int rootId, int rootOutgoingLink,
                                     int stoppingVertex, int stoppingVertexIncomingLink);
};

} // namespace bm