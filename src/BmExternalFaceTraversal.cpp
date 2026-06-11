#include "bm/BmExternalFaceTraversal.hpp"

#include <stdexcept>

namespace bm {

BmExternalFaceTraversal::BmExternalFaceTraversal(const BmPartialEmbedding& embedding)
    : embedding_(&embedding) {}

BmExternalFacePosition BmExternalFaceTraversal::successor(BmExternalFacePosition position) const {
    validatePosition(position);

    const int currentVertex = position.internalVertexId;

    const int nextVertex = embedding_->externalFaceNeighbor(currentVertex, 1 - position.linkIndex);

    if (nextVertex == -1) {
        throw std::logic_error("External-face position has no neighbor.");
    }

    const int firstNeighbor = embedding_->externalFaceNeighbor(nextVertex, 0);

    const int secondNeighbor = embedding_->externalFaceNeighbor(nextVertex, 1);

    int nextIncomingLink = -1;

    if (firstNeighbor == currentVertex && secondNeighbor == currentVertex) {
        nextIncomingLink = position.linkIndex;
    } else if (firstNeighbor == currentVertex) {
        nextIncomingLink = 0;
    } else if (secondNeighbor == currentVertex) {
        nextIncomingLink = 1;
    } else {
        throw std::logic_error("External-face neighbor relation is not symmetric.");
    }

    BmExternalFacePosition result;
    result.internalVertexId = nextVertex;
    result.linkIndex = nextIncomingLink;

    return result;
}


void BmExternalFaceTraversal::validatePosition(BmExternalFacePosition position) const {
    if (position.internalVertexId < 0 ||
        position.internalVertexId >= embedding_->internalVertexCount()) {
        throw std::out_of_range("Invalid external-face position vertex.");
    }

    if (position.linkIndex < 0 || position.linkIndex > 1) {
        throw std::out_of_range("External face link index must be 0 or 1.");
    }

    const int neighbor =
        embedding_->externalFaceNeighbor(position.internalVertexId, 1 - position.linkIndex);

    if (neighbor == -1) {
        throw std::invalid_argument("External face position has no neighbor.");
    }
}

} // namespace bm