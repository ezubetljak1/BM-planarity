#include "bm/BmKuratowskiExtractionContext.hpp"

#include <stdexcept>

namespace bm {

BmKuratowskiExtractionContext BmKuratowskiExtractionContextBuilder::initialize(
    const BmEmbeddingState& state,
    const BmWalkdownFailure& failure
) {
    state.validateVertex(failure.currentVertex);

    int centralRootId = failure.topRootId;

    // If Walkdown could not descend into a pertinent child bicomp, that
    // blocked child is the principal bicomp of Minor A. When Walkdown
    // stopped with a non-empty merge stack, the principal bicomp is the
    // root stored at the top of that stack.
    if (failure.reason == BmWalkdownFailureReason::BlockedChildBicomp) {
        centralRootId = failure.blockingChildRootId;
    } else if (!failure.mergeStack.empty()) {
        centralRootId = failure.mergeStack.back().rootId;
    }

    const BmBicompRoot& root = state.bicompRoot(centralRootId);
    const int rootInternalVertexId = root.internalRootVertexId;

    BmRealExternalFaceTraversal traversal(state.partialEmbedding());

    BmKuratowskiExtractionContext result;
    result.currentVertex = failure.currentVertex;
    result.centralRootId = centralRootId;
    result.centralRootInternalVertexId = rootInternalVertexId;
    result.x = findFirstActiveOnSide(
        state,
        traversal,
        rootInternalVertexId,
        1,
        failure.currentVertex
    );
    result.y = findFirstActiveOnSide(
        state,
        traversal,
        rootInternalVertexId,
        0,
        failure.currentVertex
    );
    result.pertinentVertex = findPertinentVertexOnLowerPath(
        state,
        traversal,
        result.x,
        result.y,
        failure.currentVertex
    );
    result.minorAConfiguration = root.parentVertex != failure.currentVertex;

    return result;
}

BmRealExternalFacePosition BmKuratowskiExtractionContextBuilder::findFirstActiveOnSide(
    const BmEmbeddingState& state,
    const BmRealExternalFaceTraversal& traversal,
    int rootInternalVertexId,
    int rootIncomingLink,
    int currentVertex
) {
    BmRealExternalFacePosition position = traversal.successor(
        {rootInternalVertexId, rootIncomingLink}
    );

    const int maxSteps = state.partialEmbedding().halfEdgeCount() + 1;

    for (int step = 0; step < maxSteps; ++step) {
        if (position.internalVertexId == rootInternalVertexId) {
            throw std::logic_error(
                "Kuratowski context initialization found no active external-face vertex."
            );
        }

        if (state.isInternalBicompRootVertex(position.internalVertexId)) {
            throw std::logic_error(
                "Kuratowski context initialization encountered an unexpected virtual root."
            );
        }

        const int vertex = state.originalVertexForInternalVertex(position.internalVertexId);

        if (!state.isInactive(vertex, currentVertex)) {
            return position;
        }

        position = traversal.successor(position);
    }

    throw std::logic_error(
        "Real external-face traversal did not find an active vertex before exceeding its guard."
    );
}

int BmKuratowskiExtractionContextBuilder::findPertinentVertexOnLowerPath(
    const BmEmbeddingState& state,
    const BmRealExternalFaceTraversal& traversal,
    BmRealExternalFacePosition x,
    BmRealExternalFacePosition y,
    int currentVertex
) {
    if (x.internalVertexId == y.internalVertexId) {
        if (state.isInternalBicompRootVertex(x.internalVertexId)) {
            return -1;
        }

        const int vertex = state.originalVertexForInternalVertex(x.internalVertexId);
        return state.isPertinent(vertex, currentVertex) ? vertex : -1;
    }

    BmRealExternalFacePosition position = traversal.successor(x);
    const int maxSteps = state.partialEmbedding().halfEdgeCount() + 1;

    for (int step = 0; step < maxSteps; ++step) {
        if (position.internalVertexId == y.internalVertexId) {
            return -1;
        }

        if (state.isInternalBicompRootVertex(position.internalVertexId)) {
            throw std::logic_error(
                "Lower X-Y external-face traversal unexpectedly reached a virtual root."
            );
        }

        const int vertex = state.originalVertexForInternalVertex(position.internalVertexId);

        if (state.isPertinent(vertex, currentVertex)) {
            return vertex;
        }

        position = traversal.successor(position);
    }

    throw std::logic_error(
        "Lower X-Y external-face traversal did not reach the second stopping vertex."
    );
}

} // namespace bm
