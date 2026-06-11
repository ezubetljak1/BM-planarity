#pragma once

#include "bm/BmEmbeddingOperations.hpp"
#include "bm/BmEmbeddingState.hpp"

namespace bm {

struct BmWalkdownResult {
    bool completed = true;
};

class BmWalkdown {
public:
    BmWalkdownResult run(BmEmbeddingState& state, int currentVertex, int rootId) const;

private:
    static bool isRootPosition(const BmEmbeddingState& state, BmExternalFacePosition position);

    static bool isSameInternalVertex(BmExternalFacePosition first, BmExternalFacePosition second);

    static bool isInternallyActivePosition(const BmEmbeddingState& state,
                                           BmExternalFacePosition position, int currentVertex);

    static bool isPertinentPosition(const BmEmbeddingState& state, BmExternalFacePosition position,
                                    int currentVertex);

    static int originalVertexAt(const BmEmbeddingState& state, BmExternalFacePosition position);
};

} // namespace bm