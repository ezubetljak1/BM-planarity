#include "bm/BmKuratowskiPathMarker.hpp"

#include <stdexcept>

namespace bm {

BmKuratowskiPathMarker::BmKuratowskiPathMarker(
    const BmEmbeddingState& state
)
    : state_(&state),
      markedOriginalEdges_(
          static_cast<std::size_t>(state.graph().edgeCount()),
          false
      ) {
}

void BmKuratowskiPathMarker::markOriginalEdge(int edgeId) {
    validateOriginalEdge(edgeId);
    markedOriginalEdges_[static_cast<std::size_t>(edgeId)] = true;
}

void BmKuratowskiPathMarker::markDfsPath(
    int ancestorVertex,
    int descendantVertex
) {
    state_->validateVertex(ancestorVertex);
    state_->validateVertex(descendantVertex);

    const DfsInfo& dfsInfo = state_->dfsInfo();
    int current = descendantVertex;

    while (current != ancestorVertex) {
        const int edgeId = dfsInfo.parentEdgeId[static_cast<std::size_t>(current)];
        const int parent = dfsInfo.parent[static_cast<std::size_t>(current)];

        if (edgeId == -1 || parent == -1) {
            throw std::invalid_argument(
                "Requested DFS path does not connect descendant to ancestor."
            );
        }

        markOriginalEdge(edgeId);
        current = parent;
    }
}

void BmKuratowskiPathMarker::markRealExternalFacePath(
    BmRealExternalFacePosition start,
    int endInternalVertexId
) {
    const BmPartialEmbedding& embedding = state_->partialEmbedding();

    if (endInternalVertexId < 0
        || endInternalVertexId >= embedding.internalVertexCount()) {
        throw std::out_of_range("Invalid external-face path endpoint.");
    }

    BmRealExternalFaceTraversal traversal(embedding);
    BmRealExternalFacePosition current = start;
    int steps = 0;

    do {
        const int outgoingHalfEdgeId = embedding.externalFaceHalfEdge(
            current.internalVertexId,
            1 - current.incomingLinkIndex
        );

        const int embeddedEdgeId = embedding.halfEdge(outgoingHalfEdgeId).embeddedEdgeId;
        const int originalEdgeId = embedding.embeddedEdge(embeddedEdgeId).originalEdgeId;

        markOriginalEdge(originalEdgeId);
        current = traversal.successor(current);
        ++steps;

        if (steps > embedding.halfEdgeCount() + 1) {
            throw std::logic_error(
                "Real external-face path did not reach its endpoint."
            );
        }
    } while (current.internalVertexId != endInternalVertexId);
}

bool BmKuratowskiPathMarker::isOriginalEdgeMarked(int edgeId) const {
    validateOriginalEdge(edgeId);
    return markedOriginalEdges_[static_cast<std::size_t>(edgeId)];
}

std::vector<int> BmKuratowskiPathMarker::markedOriginalEdgeIds() const {
    std::vector<int> result;

    for (int edgeId = 0; edgeId < static_cast<int>(markedOriginalEdges_.size()); ++edgeId) {
        if (markedOriginalEdges_[static_cast<std::size_t>(edgeId)]) {
            result.push_back(edgeId);
        }
    }

    return result;
}

void BmKuratowskiPathMarker::validateOriginalEdge(int edgeId) const {
    if (edgeId < 0 || edgeId >= state_->graph().edgeCount()) {
        throw std::out_of_range("Invalid original edge id.");
    }
}

} // namespace bm
