#pragma once

#include "bm/BmEmbeddingState.hpp"

#include <optional>

namespace bm {

struct BmOriginalBackEdgeConnection {
    int edgeId = -1;
    int ancestorVertex = -1;
    int descendantVertex = -1;
};

class BmKuratowskiConnectionFinder {
public:
    static std::optional<BmOriginalBackEdgeConnection> findToSubtree(
        const BmEmbeddingState& state,
        int ancestorVertex,
        int subtreeRootVertex
    );

    static std::optional<BmOriginalBackEdgeConnection> findToCurrentVertex(
        const BmEmbeddingState& state,
        int currentVertex,
        int cutVertex
    );

    static std::optional<BmOriginalBackEdgeConnection> findToAncestor(
        const BmEmbeddingState& state,
        int currentVertex,
        int cutVertex
    );

    static bool isInSubtree(
        const DfsInfo& dfsInfo,
        int subtreeRootVertex,
        int vertex
    );
};

} // namespace bm
