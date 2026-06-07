#include "bm/BmEmbeddingOperations.hpp"

#include <stdexcept>
#include <string>

namespace bm {

namespace {

void validateLinkIndex(int linkIndex, const char* fieldName) {
    if (linkIndex < 0 || linkIndex > 1)
        throw std::out_of_range(std::string(fieldName) + " must be 0 or 1.");
}

} // namespace

void BmEmbeddingOperations::mergeTopBiconnectedComponent(BmEmbeddingState& state,
                                                         std::vector<BmMergeFrame>& mergeStack) {
    if (mergeStack.empty())
        throw std::logic_error("Cannot merge from an empty stack.");

    const BmMergeFrame frame = mergeStack.back();
    mergeStack.pop_back();

    (void) state.vertexState(frame.cutVertex); 

    validateLinkIndex(frame.cutVertexIncomingLink, "Cut-vertex incoming link");

    validateLinkIndex(frame.rootOutgoingLink, "Root outgoing link");

    BmBicompRoot& root = state.bicompRoot(frame.rootId);

    if (!root.active)
        throw std::logic_error("Cannot merge an inactive bicomp root.");

    if (root.parentVertex != frame.cutVertex)
        throw std::logic_error("Merge frame cut vertex does not match bicomp root parent.");

    BmPartialEmbedding& embedding = state.partialEmbedding();

    const int targetInternalVertexId = embedding.originalInternalVertex(frame.cutVertex);

    const int sourceInternalVertexId = root.internalRootVertexId;

    int rootOutgoingLink = frame.rootOutgoingLink;

    // Appendix B:
    // if r_in == r^c_out, invert orientation of r^c
    // and set sign of root edge to -1.
    if (frame.cutVertexIncomingLink == rootOutgoingLink) {
        embedding.reverseAdjacencyOrientation(sourceInternalVertexId);

        root.rootEdgeSign = -1;

        embedding.setEmbeddedEdgeSign(root.embeddedTreeEdgeId, -1);

        rootOutgoingLink = 1 - rootOutgoingLink;
    }

    // For each outgoing half-edge stored at virtual root r^c:
    // update it so it is now incident to real cut vertex r.
    embedding.redirectAdjacencyToVertex(sourceInternalVertexId, targetInternalVertexId);

    state.removeExpectedFirstPertinentRoot(frame.cutVertex, root.id);

    state.removeSeparatedDfsChild(frame.cutVertex, root.childVertex);

    embedding.spliceAdjacencyLists(targetInternalVertexId, sourceInternalVertexId,
                                   frame.cutVertexIncomingLink, rootOutgoingLink);

    state.deactivateBicompRoot(root.id);
}

void BmEmbeddingOperations::mergeAllBiconnectedComponents(BmEmbeddingState& state,
                                                          std::vector<BmMergeFrame>& mergeStack) {
    while (!mergeStack.empty())
        mergeTopBiconnectedComponent(state, mergeStack);
}

} // namespace bm