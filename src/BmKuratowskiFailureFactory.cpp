#include "bm/BmKuratowskiFailureFactory.hpp"

#include <stdexcept>

namespace bm {

BmWalkdownFailure BmKuratowskiFailureFactory::fromUnembeddedBackedge(
    const BmEmbeddingState& state,
    int currentVertex,
    const DfsBackEdge& backEdge
) {
    state.validateVertex(currentVertex);

    if (backEdge.ancestor != currentVertex) {
        throw std::invalid_argument(
            "Unembedded back edge ancestor must equal the current BM vertex."
        );
    }

    if (state.isOriginalEdgeEmbedded(backEdge.edgeId)) {
        throw std::invalid_argument(
            "Cannot create a non-planarity failure from an already embedded edge."
        );
    }

    const int child = firstChildBelowAncestor(
        state.dfsInfo(),
        currentVertex,
        backEdge.descendant
    );

    const int rootId = state.rootForChild(child);

    if (rootId == -1) {
        throw std::logic_error(
            "Unembedded back edge subtree has no bicomp root."
        );
    }

    BmWalkdownFailure failure;
    failure.reason = BmWalkdownFailureReason::UnembeddedBackedge;
    failure.currentVertex = currentVertex;
    failure.topRootId = rootId;
    failure.blockingVertex = backEdge.descendant;
    failure.blockingChildRootId = rootId;
    failure.unembeddedEdgeId = backEdge.edgeId;
    failure.unembeddedDescendantVertex = backEdge.descendant;

    return failure;
}

int BmKuratowskiFailureFactory::firstChildBelowAncestor(
    const DfsInfo& dfsInfo,
    int ancestor,
    int descendant
) {
    if (ancestor < 0 || ancestor >= dfsInfo.vertexCount
        || descendant < 0 || descendant >= dfsInfo.vertexCount) {
        throw std::out_of_range("Invalid DFS vertex id.");
    }

    int current = descendant;

    while (current != -1 && dfsInfo.parent[current] != ancestor) {
        current = dfsInfo.parent[current];
    }

    if (current == -1) {
        throw std::invalid_argument(
            "Back-edge descendant does not belong to the ancestor subtree."
        );
    }

    return current;
}

} // namespace bm
