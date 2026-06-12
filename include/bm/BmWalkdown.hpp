#pragma once

#include "bm/BmEmbeddingOperations.hpp"
#include "bm/BmEmbeddingState.hpp"

#include <optional>
#include <vector>

namespace bm {

enum class BmWalkdownFailureReason {
    BlockedChildBicomp,
    NonEmptyMergeStack,
    UnembeddedBackedge
};

struct BmWalkdownFailure {
    BmWalkdownFailureReason reason = BmWalkdownFailureReason::BlockedChildBicomp;

    int currentVertex = -1;
    int topRootId = -1;
    int rootOutgoingLink = -1;

    int blockingVertex = -1;
    int blockingIncomingLink = -1;
    int blockingChildRootId = -1;

    // Filled when the decision phase reports an unembedded back edge after Walkdown.
    int unembeddedEdgeId = -1;
    int unembeddedDescendantVertex = -1;

    std::vector<BmMergeFrame> mergeStack;
};

struct BmWalkdownResult {
    bool completed = true;
    std::optional<BmWalkdownFailure> failure;
};

class BmWalkdown {
public:
    BmWalkdownResult run(BmEmbeddingState& state, int currentVertex, int rootId) const;

private:
    static bool isRootPosition(const BmEmbeddingState& state, BmExternalFacePosition position);


    static bool isInternallyActivePosition(const BmEmbeddingState& state,
                                           BmExternalFacePosition position, int currentVertex);

    static bool isPertinentPosition(const BmEmbeddingState& state, BmExternalFacePosition position,
                                    int currentVertex);

    static int originalVertexAt(const BmEmbeddingState& state, BmExternalFacePosition position);
};

} // namespace bm