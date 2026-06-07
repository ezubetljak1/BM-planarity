#include "bm/BmWalkdown.hpp"

#include "bm/BmExternalFaceTraversal.hpp"

#include <stdexcept>
#include <vector>

namespace bm {

BmWalkdownResult BmWalkdown::run(BmEmbeddingState& state, int currentVertex, int rootId) const {
    (void)state.vertexState(currentVertex);

    BmBicompRoot& topRoot = state.bicompRoot(rootId);

    if (!topRoot.active) {
        throw std::logic_error("Walkdown requires an active bicomp root.");
    }

    const int topRootInternalVertexId = topRoot.internalRootVertexId;

    const int child = topRoot.childVertex;

    // The main BM driver calls Walkdown only for pertinent DFS subtrees.
    // Keep the operation safe when called directly from a test or helper.
    if (!state.hasPertinentRoots(child)) {
        return {true};
    }

    const int currentDfi = state.dfsInfo().dfsIndex[currentVertex];

    BmExternalFaceTraversal traversal(state.partialEmbedding());

    std::vector<BmMergeFrame> mergeStack;

    for (int rootOutgoingLink = 0; rootOutgoingLink <= 1; ++rootOutgoingLink) {
        mergeStack.clear();

        BmExternalFacePosition position;
        position.internalVertexId = topRootInternalVertexId;
        position.linkIndex = 1 - rootOutgoingLink;

        position = traversal.successor(position);

        int traversalSteps = 0;

        while (position.internalVertexId != topRootInternalVertexId) {

            ++traversalSteps;

            if (traversalSteps > state.partialEmbedding().halfEdgeCount() + 1) {
                throw std::logic_error("Walkdown traversal did not return to the bicomp root.");
            }

            if (isRootPosition(state, position)) {
                throw std::logic_error("Unexpected virtual root during Walkdown traversal.");
            }

            const int vertex = originalVertexAt(state, position);

            if (state.isBackedgeEndpointForCurrentVertex(vertex, currentVertex)) {
                BmEmbeddingOperations::mergeAllBiconnectedComponents(state, mergeStack);

                BmEmbeddingOperations::embedBackEdge(state, rootId, rootOutgoingLink, vertex,
                                                     position.linkIndex, currentVertex);
            }

            if (state.hasPertinentRoots(vertex)) {
                const int childRootId = state.firstPertinentRoot(vertex);

                const BmBicompRoot& childRoot = state.bicompRoot(childRootId);

                BmExternalFacePosition childRootFirst;
                childRootFirst.internalVertexId = childRoot.internalRootVertexId;
                childRootFirst.linkIndex = 1;

                BmExternalFacePosition childRootSecond;
                childRootSecond.internalVertexId = childRoot.internalRootVertexId;
                childRootSecond.linkIndex = 0;

                const BmExternalFacePosition x =
                    BmEmbeddingOperations::getActiveSuccessorOnExternalFace(state, childRootFirst,
                                                                            currentVertex);

                const BmExternalFacePosition y =
                    BmEmbeddingOperations::getActiveSuccessorOnExternalFace(state, childRootSecond,
                                                                            currentVertex);

                bool chooseX = false;
                bool selected = false;

                if (isInternallyActivePosition(state, x, currentVertex)) {
                    chooseX = true;
                    selected = true;
                } else if (isInternallyActivePosition(state, y, currentVertex)) {
                    chooseX = false;
                    selected = true;
                } else if (isPertinentPosition(state, x, currentVertex)) {
                    chooseX = true;
                    selected = true;
                } else if (isPertinentPosition(state, y, currentVertex)) {
                    chooseX = false;
                    selected = true;
                }

                if (!selected) {
                    // Both external-face directions lead to
                    // non-pertinent externally active vertices.
                    // This is the immediate Walkdown blocking
                    // configuration from Section 5.2.
                    return {false};
                }

                BmMergeFrame frame;
                frame.cutVertex = vertex;
                frame.cutVertexIncomingLink = position.linkIndex;
                frame.rootId = childRootId;
                frame.rootOutgoingLink = chooseX ? 0 : 1;

                mergeStack.push_back(frame);

                position = chooseX ? x : y;

                continue;
            }

            if (state.isInactive(vertex, currentVertex)) {
                position = traversal.successor(position);

                continue;
            }

            // The vertex is externally active and non-pertinent:
            // it is a stopping vertex. Walkdown must stop here

            break;
        }

        if (!mergeStack.empty()) {
            return {false};
        }
    }

    return {true};
}

bool BmWalkdown::isRootPosition(const BmEmbeddingState& state, BmExternalFacePosition position) {
    return state.isInternalBicompRootVertex(position.internalVertexId);
}

bool BmWalkdown::isSameInternalVertex(BmExternalFacePosition first, BmExternalFacePosition second) {
    return first.internalVertexId == second.internalVertexId;
}

bool BmWalkdown::isInternallyActivePosition(const BmEmbeddingState& state,
                                            BmExternalFacePosition position, int currentVertex) {
    if (isRootPosition(state, position)) {
        return false;
    }

    return state.isInternallyActive(originalVertexAt(state, position), currentVertex);
}

bool BmWalkdown::isPertinentPosition(const BmEmbeddingState& state, BmExternalFacePosition position,
                                     int currentVertex) {
    if (isRootPosition(state, position)) {
        return false;
    }

    return state.isPertinent(originalVertexAt(state, position), currentVertex);
}

int BmWalkdown::originalVertexAt(const BmEmbeddingState& state, BmExternalFacePosition position) {
    if (isRootPosition(state, position)) {
        throw std::invalid_argument("Expected original internal vertex, got bicomp root.");
    }

    return state.originalVertexForInternalVertex(position.internalVertexId);
}

} // namespace bm