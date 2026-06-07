#pragma once

#include "bm/BmEmbeddingState.hpp"
#include "bm/BmExternalFaceTraversal.hpp"

namespace bm {

class BmWalkup {
public:
    void run(BmEmbeddingState& state, int currentVertex, int descendantVertex) const;

    void run(BmEmbeddingState& state, int currentVertex, int descendantVertex,
             int originalEdgeId) const;

private:
    void runTraversal(BmEmbeddingState& state, int currentVertex, int descendantVertex) const;

    static BmExternalFacePosition originalPosition(const BmEmbeddingState& state,
                                                   int originalVertex, int linkIndex);

    static bool isCurrentOriginalVertex(const BmEmbeddingState& state,
                                        BmExternalFacePosition position, int currentVertex);

    static bool isRootPosition(const BmEmbeddingState& state, BmExternalFacePosition position);
};

} // namespace bm