#pragma once

#include <vector>

#include "bm/BmEmbeddingState.hpp"

namespace bm {

struct BmMergeFrame {
    // Non-virtual cut vertex r.
    int cutVertex = -1;

    // r_in from Appendix B.
    int cutVertexIncomingLink = -1;

    // Virtual root r^c represented by a BmBicompRoot id.
    int rootId = -1;

    // r^c_out from Appendix B.
    int rootOutgoingLink = -1;
};

class BmEmbeddingOperations {
public:
    static void mergeTopBiconnectedComponent(BmEmbeddingState& state,
                                             std::vector<BmMergeFrame>& mergeStack);

    static void mergeAllBiconnectedComponents(BmEmbeddingState& state,
                                              std::vector<BmMergeFrame>& mergeStack);
};

} // namespace bm