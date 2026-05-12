#include "bm/BmExternalFaceTraversal.hpp"

#include <stdexcept>

namespace bm {

BmExternalFaceTraversal::BmExternalFaceTraversal(const BmPartialEmbedding& embedding)
    : embedding_(&embedding) {}

BmExternalFacePosition BmExternalFaceTraversal::successor(BmExternalFacePosition position) const {
    validatePosition(position);

    const int currentVertex = position.internalVertexId;
    const int outgoingHalfEdge =
        embedding_->externalFaceHalfEdge(currentVertex, position.linkIndex);

    const BmHalfEdge& edge = embedding_->halfEdge(outgoingHalfEdge);
    const int nextVertex = edge.to;
    const int incomingTwin = edge.twin;

    const int firstLink = embedding_->externalFaceHalfEdge(currentVertex, 0);
    const int secondLink = embedding_->externalFaceHalfEdge(currentVertex, 1);

    int nextLinkIndex = -1;

    // degree - 1 special case from BM
    // when both external links of the current vertex are the same
    // preserve the side / index
    if (firstLink == secondLink)
        nextLinkIndex = position.linkIndex;
    else
        nextLinkIndex = embedding_->externalFaceLinkIndex(nextVertex, incomingTwin);

    BmExternalFacePosition result;
    result.internalVertexId = nextVertex;
    result.linkIndex = nextLinkIndex;

    return result;
}

BmExternalFaceStep BmExternalFaceTraversal::step(BmExternalFacePosition position) const {
    validatePosition(position);

    const int outgoingHalfEdge =
        embedding_->externalFaceHalfEdge(position.internalVertexId, position.linkIndex);

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

    if (maxSteps < 0)
        throw std::invalid_argument("Maximum number of steps cannot be negative.");

    std::vector<BmExternalFaceStep> result;

    BmExternalFacePosition current = start;

    for (int stepIndex = 0; stepIndex < maxSteps; ++stepIndex) {
        BmExternalFaceStep currentStep = step(current);
        result.push_back(currentStep);

        current = currentStep.successor;

        if (current.internalVertexId == start.internalVertexId &&
            current.linkIndex == start.linkIndex)
            break;
    }

    return result;
}

void BmExternalFaceTraversal::validatePosition(BmExternalFacePosition position) const {
    if (position.internalVertexId < 0 ||
        position.internalVertexId >= embedding_->internalVertexCount())
        throw std::out_of_range("Invalid external-face position vertex.");

    if (position.linkIndex < 0 || position.linkIndex > 1)
        throw std::out_of_range("External face link index must be 0 or 1.");

    const int halfEdge =
        embedding_->externalFaceHalfEdge(position.internalVertexId, position.linkIndex);

    if (halfEdge == -1)
        throw std::invalid_argument("External face position has no half-edge.");
}

} // namespace bm