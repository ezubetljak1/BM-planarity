#include "bm/BmRealExternalFaceTraversal.hpp"

#include <stdexcept>

namespace bm {

BmRealExternalFaceTraversal::BmRealExternalFaceTraversal(
    const BmPartialEmbedding& embedding
)
    : embedding_(&embedding) {
}

BmRealExternalFacePosition BmRealExternalFaceTraversal::successor(
    BmRealExternalFacePosition position
) const {
    validatePosition(position);

    const int outgoingHalfEdgeId = embedding_->externalFaceHalfEdge(
        position.internalVertexId,
        1 - position.incomingLinkIndex
    );

    const BmHalfEdge& outgoingHalfEdge = embedding_->halfEdge(outgoingHalfEdgeId);
    const int nextVertexId = outgoingHalfEdge.to;
    const int incomingTwinId = outgoingHalfEdge.twin;

    const int firstAnchor = embedding_->externalFaceHalfEdge(nextVertexId, 0);
    const int secondAnchor = embedding_->externalFaceHalfEdge(nextVertexId, 1);

    int nextIncomingLinkIndex = -1;

    // Preserve the direction bit for a singleton bicomp: both anchors are the
    // same half-edge, but the two sides still act as a two-edge external cycle.
    if (firstAnchor == secondAnchor) {
        nextIncomingLinkIndex = position.incomingLinkIndex;
    } else if (firstAnchor == incomingTwinId) {
        nextIncomingLinkIndex = 0;
    } else if (secondAnchor == incomingTwinId) {
        nextIncomingLinkIndex = 1;
    } else {
        throw std::logic_error(
            "Real external-face traversal entered a vertex through a non-anchor half-edge."
        );
    }

    return {nextVertexId, nextIncomingLinkIndex};
}

void BmRealExternalFaceTraversal::validatePosition(
    BmRealExternalFacePosition position
) const {
    if (position.internalVertexId < 0
        || position.internalVertexId >= embedding_->internalVertexCount()) {
        throw std::out_of_range("Invalid real external-face position vertex.");
    }

    if (position.incomingLinkIndex < 0 || position.incomingLinkIndex > 1) {
        throw std::out_of_range("Real external-face incoming link index must be 0 or 1.");
    }

    const int outgoingHalfEdgeId = embedding_->externalFaceHalfEdge(
        position.internalVertexId,
        1 - position.incomingLinkIndex
    );

    if (outgoingHalfEdgeId == -1) {
        throw std::invalid_argument("Real external-face position has no outgoing anchor.");
    }
}

} // namespace bm
