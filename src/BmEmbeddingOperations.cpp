#include "bm/BmEmbeddingOperations.hpp"

#include <stdexcept>
#include <string>

namespace bm {

namespace {

void validateLinkIndex(int linkIndex, const char* fieldName) {
    if (linkIndex < 0 || linkIndex > 1) {
        throw std::out_of_range(std::string(fieldName) + " must be 0 or 1.");
    }
}

} // namespace

void BmEmbeddingOperations::mergeTopBiconnectedComponent(BmEmbeddingState& state,
                                                         std::vector<BmMergeFrame>& mergeStack) {
    if (mergeStack.empty()) {
        throw std::logic_error("Cannot merge from an empty stack.");
    }

    const BmMergeFrame frame = mergeStack.back();

    mergeStack.pop_back();

    (void)state.vertexState(frame.cutVertex);

    validateLinkIndex(frame.cutVertexIncomingLink, "Cut-vertex incoming link");

    validateLinkIndex(frame.rootOutgoingLink, "Root outgoing link");

    BmBicompRoot& root = state.bicompRoot(frame.rootId);

    if (!root.active) {
        throw std::logic_error("Cannot merge inactive bicomp root.");
    }

    if (root.parentVertex != frame.cutVertex) {
        throw std::logic_error("Merge-frame cut vertex does not match root parent.");
    }

    BmPartialEmbedding& embedding = state.partialEmbedding();

    const int targetInternalVertexId = embedding.originalInternalVertex(frame.cutVertex);

    const int sourceInternalVertexId = root.internalRootVertexId;

    int rootOutgoingLink = frame.rootOutgoingLink;

    const int externalFaceVertex =
        embedding.externalFaceNeighbor(sourceInternalVertexId, 1 - rootOutgoingLink);

    embedding.setExternalFaceNeighbor(targetInternalVertexId, frame.cutVertexIncomingLink,
                                      externalFaceVertex);

    if (embedding.externalFaceNeighbor(externalFaceVertex, 0) ==
        embedding.externalFaceNeighbor(externalFaceVertex, 1)) {
        embedding.setExternalFaceNeighbor(externalFaceVertex, rootOutgoingLink,
                                          targetInternalVertexId);
    } else {
        const int externalFaceLink =
            embedding.externalFaceNeighbor(externalFaceVertex, 0) == sourceInternalVertexId ? 0 : 1;

        embedding.setExternalFaceNeighbor(externalFaceVertex, externalFaceLink,
                                          targetInternalVertexId);
    }

    if (frame.cutVertexIncomingLink == rootOutgoingLink) {
        embedding.reverseAdjacencyOrientation(sourceInternalVertexId);

        embedding.setEmbeddedEdgeSign(root.embeddedTreeEdgeId, -1);

        root.rootEdgeSign = -1;

        rootOutgoingLink = 1 - rootOutgoingLink;
    }

    embedding.redirectAdjacencyToVertex(sourceInternalVertexId, targetInternalVertexId);

    state.removeExpectedFirstPertinentRoot(frame.cutVertex, root.id);

    state.removeSeparatedDfsChild(frame.cutVertex, root.childVertex);

    embedding.spliceAdjacencyLists(targetInternalVertexId, sourceInternalVertexId,
                                   frame.cutVertexIncomingLink, rootOutgoingLink);

    state.deactivateBicompRoot(root.id);
}

void BmEmbeddingOperations::mergeAllBiconnectedComponents(BmEmbeddingState& state,
                                                          std::vector<BmMergeFrame>& mergeStack) {
    while (!mergeStack.empty()) {
        mergeTopBiconnectedComponent(state, mergeStack);
    }
}

BmExternalFacePosition BmEmbeddingOperations::getActiveSuccessorOnExternalFace(
    const BmEmbeddingState& state, BmExternalFacePosition start, int currentVertex) {
    BmExternalFaceTraversal traversal(state.partialEmbedding());

    BmExternalFacePosition current = traversal.successor(start);

    const int maxSteps = state.partialEmbedding().halfEdgeCount() + 1;

    for (int step = 0; step < maxSteps; ++step) {
        if (current.internalVertexId == start.internalVertexId) {
            return current;
        }

        if (state.isInternalBicompRootVertex(current.internalVertexId)) {
            return current;
        }

        const int vertex = state.originalVertexForInternalVertex(current.internalVertexId);

        if (!state.isInactive(vertex, currentVertex)) {
            return current;
        }

        current = traversal.successor(current);
    }

    throw std::logic_error("External-face traversal did not return to its start vertex.");
}

int BmEmbeddingOperations::embedBackEdge(BmEmbeddingState& state, int rootId, int rootOutgoingLink,
                                         int descendantVertex, int descendantIncomingLink,
                                         int currentVertex) {
    validateLinkIndex(rootOutgoingLink, "Root outgoing link");

    validateLinkIndex(descendantIncomingLink, "Descendant incoming link");

    BmBicompRoot& root = state.bicompRoot(rootId);

    const int originalEdgeId = state.pendingBackedgeOriginalEdgeId(descendantVertex, currentVertex);

    BmPartialEmbedding& embedding = state.partialEmbedding();

    const int descendantInternalVertexId = embedding.originalInternalVertex(descendantVertex);

    const int embeddedEdgeId = embedding.addExternalFaceEdge(
        root.internalRootVertexId, rootOutgoingLink, descendantInternalVertexId,
        descendantIncomingLink, originalEdgeId, false);

    state.registerEmbeddedOriginalEdge(originalEdgeId, embeddedEdgeId);

    state.clearBackedgeFlag(descendantVertex);

    return embeddedEdgeId;
}

int BmEmbeddingOperations::embedShortCircuitEdge(BmEmbeddingState& state, int rootId,
                                                 int rootOutgoingLink, int stoppingVertex,
                                                 int stoppingVertexIncomingLink) {
    validateLinkIndex(rootOutgoingLink, "Root outgoing link");

    validateLinkIndex(stoppingVertexIncomingLink, "Stopping-vertex incoming link");

    BmBicompRoot& root = state.bicompRoot(rootId);

    BmPartialEmbedding& embedding = state.partialEmbedding();

    const int stoppingInternalVertexId = embedding.originalInternalVertex(stoppingVertex);

    embedding.shortcutExternalFacePath(root.internalRootVertexId, rootOutgoingLink,
                                       stoppingInternalVertexId, stoppingVertexIncomingLink);

    return -1;
}

} // namespace bm