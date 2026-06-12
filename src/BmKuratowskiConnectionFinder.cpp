#include "bm/BmKuratowskiConnectionFinder.hpp"

#include <stdexcept>
#include <vector>

namespace bm {

namespace {

std::vector<bool> allowedSeparatedDescendants(
    const BmEmbeddingState& state,
    int cutVertex
) {
    const DfsInfo& dfsInfo = state.dfsInfo();
    const int n = dfsInfo.vertexCount;

    std::vector<int> difference(static_cast<std::size_t>(n + 1), 0);

    const auto markInterval = [&](int root) {
        const int begin = dfsInfo.dfsIndex[static_cast<std::size_t>(root)];
        const int end = dfsInfo.subtreeEndDfi[static_cast<std::size_t>(root)];

        ++difference[static_cast<std::size_t>(begin)];
        --difference[static_cast<std::size_t>(end + 1)];
    };

    // A future-pertinent connection may be direct from the cut vertex or
    // enter one of its still-separated DFS-child subtrees. Descendants that
    // have already been merged into the cut vertex bicomp must not be
    // considered here; the reference _FindUnembeddedEdgeToAncestor routine
    // scans only the direct least-ancestor edge and separated DFS children.
    for (int child : state.separatedDfsChildren(cutVertex)) {
        markInterval(child);
    }

    std::vector<bool> allowed(static_cast<std::size_t>(n), false);
    int active = 0;

    for (int dfi = 0; dfi < n; ++dfi) {
        active += difference[static_cast<std::size_t>(dfi)];

        if (active > 0) {
            const int vertex = dfsInfo.vertexAtDfsIndex[static_cast<std::size_t>(dfi)];
            allowed[static_cast<std::size_t>(vertex)] = true;
        }
    }

    allowed[static_cast<std::size_t>(cutVertex)] = true;
    return allowed;
}

} // namespace

std::optional<BmOriginalBackEdgeConnection>
BmKuratowskiConnectionFinder::findToSubtree(
    const BmEmbeddingState& state,
    int ancestorVertex,
    int subtreeRootVertex
) {
    state.validateVertex(ancestorVertex);
    state.validateVertex(subtreeRootVertex);

    const DfsInfo& dfsInfo = state.dfsInfo();
    std::optional<BmOriginalBackEdgeConnection> result;

    for (const DfsBackEdge& edge : dfsInfo.backEdges) {
        if (edge.ancestor != ancestorVertex
            || !isInSubtree(dfsInfo, subtreeRootVertex, edge.descendant)) {
            continue;
        }

        if (!result.has_value()
            || dfsInfo.dfsIndex[static_cast<std::size_t>(edge.descendant)]
                < dfsInfo.dfsIndex[static_cast<std::size_t>(result->descendantVertex)]) {
            result = BmOriginalBackEdgeConnection{
                edge.edgeId,
                edge.ancestor,
                edge.descendant
            };
        }
    }

    return result;
}

std::optional<BmOriginalBackEdgeConnection>
BmKuratowskiConnectionFinder::findToCurrentVertex(
    const BmEmbeddingState& state,
    int currentVertex,
    int cutVertex
) {
    state.validateVertex(currentVertex);
    state.validateVertex(cutVertex);

    const DfsInfo& dfsInfo = state.dfsInfo();

    for (const DfsBackEdge& edge : dfsInfo.backEdges) {
        if (edge.ancestor == currentVertex && edge.descendant == cutVertex) {
            return BmOriginalBackEdgeConnection{
                edge.edgeId,
                edge.ancestor,
                edge.descendant
            };
        }
    }

    const auto& roots = state.vertexState(cutVertex).pertinentRoots;

    if (roots.empty()) {
        return std::nullopt;
    }

    const int subtreeRoot = state.bicompRoot(roots.front()).childVertex;
    return findToSubtree(state, currentVertex, subtreeRoot);
}

std::optional<BmOriginalBackEdgeConnection>
BmKuratowskiConnectionFinder::findToAncestor(
    const BmEmbeddingState& state,
    int currentVertex,
    int cutVertex
) {
    state.validateVertex(currentVertex);
    state.validateVertex(cutVertex);

    const DfsInfo& dfsInfo = state.dfsInfo();
    const std::vector<bool> allowed = allowedSeparatedDescendants(state, cutVertex);
    const int currentDfi = dfsInfo.dfsIndex[static_cast<std::size_t>(currentVertex)];

    std::optional<BmOriginalBackEdgeConnection> result;

    for (const DfsBackEdge& edge : dfsInfo.backEdges) {
        if (dfsInfo.dfsIndex[static_cast<std::size_t>(edge.ancestor)] >= currentDfi
            || !allowed[static_cast<std::size_t>(edge.descendant)]) {
            continue;
        }

        if (!result.has_value()
            || dfsInfo.dfsIndex[static_cast<std::size_t>(edge.ancestor)]
                < dfsInfo.dfsIndex[static_cast<std::size_t>(result->ancestorVertex)]) {
            result = BmOriginalBackEdgeConnection{
                edge.edgeId,
                edge.ancestor,
                edge.descendant
            };
        }
    }

    return result;
}

bool BmKuratowskiConnectionFinder::isInSubtree(
    const DfsInfo& dfsInfo,
    int subtreeRootVertex,
    int vertex
) {
    if (subtreeRootVertex < 0 || subtreeRootVertex >= dfsInfo.vertexCount
        || vertex < 0 || vertex >= dfsInfo.vertexCount) {
        throw std::out_of_range("Invalid DFS vertex id.");
    }

    const int subtreeBegin = dfsInfo.dfsIndex[static_cast<std::size_t>(subtreeRootVertex)];
    const int subtreeEnd = dfsInfo.subtreeEndDfi[static_cast<std::size_t>(subtreeRootVertex)];
    const int vertexDfi = dfsInfo.dfsIndex[static_cast<std::size_t>(vertex)];

    return subtreeBegin <= vertexDfi && vertexDfi <= subtreeEnd;
}

} // namespace bm
