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

BmExternalFaceStep BmExternalFaceTraversal::step(BmExternalFacePosition position) const {
    validatePosition(position);

    const int outgoingHalfEdge =
        embedding_->externalFaceHalfEdge(position.internalVertexId, 1 - position.linkIndex);

    const BmHalfEdge& halfEdge = embedding_->halfEdge(outgoingHalfEdge);

    BmExternalFaceStep result;
    result.fromInternalVertexId = halfEdge.from;
    result.toInternalVertexId = halfEdge.to;
    result.usedHalfEdgeId = outgoingHalfEdge;
    result.successor = successor(position);

    return result;
}

std::vector<BmExternalFaceStep> BmExternalFaceTraversal::collectCycle(BmExternalFacePosition start,
                                                                      int maxSteps) const {
    validatePosition(start);

    if (maxSteps < 0) {
        throw std::invalid_argument("Maximum number of steps cannot be negative.");
    }

    std::vector<BmExternalFaceStep> result;

    BmExternalFacePosition current = start;

    for (int stepIndex = 0; stepIndex < maxSteps; ++stepIndex) {
        BmExternalFaceStep currentStep = step(current);

        result.push_back(currentStep);

        current = currentStep.successor;

        if (current.internalVertexId == start.internalVertexId &&
            current.linkIndex == start.linkIndex) {
            break;
        }
    }

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