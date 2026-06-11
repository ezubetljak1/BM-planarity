#pragma once

#include "bm/BmEmbeddingState.hpp"
#include "bm/BmRealExternalFaceTraversal.hpp"
#include "bm/BmWalkdown.hpp"

namespace bm {

struct BmKuratowskiExtractionContext {
    int currentVertex = -1;
    int centralRootId = -1;
    int centralRootInternalVertexId = -1;

    BmRealExternalFacePosition x;
    BmRealExternalFacePosition y;

    // First pertinent vertex on the lower X-Y external-face path, if one exists.
    int pertinentVertex = -1;

    bool minorAConfiguration = false;
};

class BmKuratowskiExtractionContextBuilder {
public:
    static BmKuratowskiExtractionContext initialize(
        const BmEmbeddingState& state,
        const BmWalkdownFailure& failure
    );

private:
    static BmRealExternalFacePosition findFirstActiveOnSide(
        const BmEmbeddingState& state,
        const BmRealExternalFaceTraversal& traversal,
        int rootInternalVertexId,
        int rootIncomingLink,
        int currentVertex
    );

    static int findPertinentVertexOnLowerPath(
        const BmEmbeddingState& state,
        const BmRealExternalFaceTraversal& traversal,
        BmRealExternalFacePosition x,
        BmRealExternalFacePosition y,
        int currentVertex
    );
};

} // namespace bm
