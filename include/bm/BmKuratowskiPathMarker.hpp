#pragma once

#include "bm/BmEmbeddingState.hpp"
#include "bm/BmRealExternalFaceTraversal.hpp"

#include <vector>

namespace bm {

class BmKuratowskiPathMarker {
public:
    explicit BmKuratowskiPathMarker(const BmEmbeddingState& state);

    void markOriginalEdge(int edgeId);
    void markDfsPath(int ancestorVertex, int descendantVertex);

    void markRealExternalFacePath(
        BmRealExternalFacePosition start,
        int endInternalVertexId
    );

    bool isOriginalEdgeMarked(int edgeId) const;
    std::vector<int> markedOriginalEdgeIds() const;

private:
    const BmEmbeddingState* state_ = nullptr;
    std::vector<bool> markedOriginalEdges_;

    void validateOriginalEdge(int edgeId) const;
};

} // namespace bm
