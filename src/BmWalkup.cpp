#include "bm/BmWalkup.hpp"
#include "bm/BmEmbeddingState.hpp"

#include <stdexcept>

namespace bm {

void BmWalkup::run(BmEmbeddingState& state, int currentVertex, int descendantVertex) const {
    state.validateVertex(currentVertex);
    state.validateVertex(descendantVertex);

    state.markBackedgeFlag(descendantVertex, currentVertex);

    const BmPartialEmbedding& embedding = state.partialEmbedding();
    BmExternalFaceTraversal traversal(embedding);

    BmExternalFacePosition x = originalPosition(state, descendantVertex, 1);
    BmExternalFacePosition y = originalPosition(state, descendantVertex, 0);

    while (!isCurrentOriginalVertex(state, x, currentVertex)) {
        if (state.isInternalVertexVisitedInStep(x.internalVertexId, currentVertex) ||
            state.isInternalVertexVisitedInStep(y.internalVertexId, currentVertex))
            break;

        state.markInternalVertexVisitedInStep(x.internalVertexId, currentVertex);
        state.markInternalVertexVisitedInStep(y.internalVertexId, currentVertex);

        BmExternalFacePosition rootPosition;

        if (isRootPosition(state, x)) {
            rootPosition = x;
        } else if (isRootPosition(state, y)) {
            rootPosition = y;
        } else {
            x = traversal.successor(x);
            y = traversal.successor(y);
            continue;
        }

        const int rootId = state.bicompRootIdForInternalVertex(rootPosition.internalVertexId);

        const BmBicompRoot& root = state.bicompRoot(rootId);

        const int child = root.childVertex;
        const int parent = state.dfsInfo().parent[child];

        if (parent == -1)
            throw std::logic_error("Bicomp root child has no DFS parent.");

        if (parent != root.parentVertex)
            throw std::logic_error("Bicomp root parent does not match DFS parent.");

        if (parent != currentVertex)
            state.addPertinentRoot(parent, rootId, currentVertex);

        x = originalPosition(state, parent, 1);
        y = originalPosition(state, parent, 0);
    }
}

BmExternalFacePosition BmWalkup::originalPosition(const BmEmbeddingState& state, int originalVertex,
                                                  int linkIndex) {
    BmExternalFacePosition position;
    position.internalVertexId = state.partialEmbedding().originalInternalVertex(originalVertex);
    position.linkIndex = linkIndex;

    return position;
}

bool BmWalkup::isCurrentOriginalVertex(const BmEmbeddingState& state,
                                       BmExternalFacePosition position, int currentVertex) {
    if (state.isInternalBicompRootVertex(position.internalVertexId))
        return false;

    return state.originalVertexForInternalVertex(position.internalVertexId) == currentVertex;
}

bool BmWalkup::isRootPosition(const BmEmbeddingState& state, BmExternalFacePosition position) {
    return state.isInternalBicompRootVertex(position.internalVertexId);
}

} // namespace bm