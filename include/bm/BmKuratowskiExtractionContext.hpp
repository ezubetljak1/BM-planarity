#pragma once

#include "bm/BmEmbeddingState.hpp"
#include "bm/BmRealExternalFaceTraversal.hpp"
#include "bm/BmWalkdown.hpp"

#include <vector>

namespace bm {

enum class BmKuratowskiObstructionMark {
    Unmarked,
    HighRxw,
    LowRxw,
    HighRyw,
    LowRyw
};

struct BmKuratowskiExtractionContext {
    int currentVertex = -1;
    int centralRootId = -1;
    int centralRootInternalVertexId = -1;

    BmRealExternalFacePosition x;
    BmRealExternalFacePosition y;

    // First pertinent vertex on the lower X-Y external-face path, if one exists.
    int pertinentVertex = -1;

    bool minorAConfiguration = false;

    // Filled by the internal X-Y-path stage used for Minors C, D and E.
    std::vector<BmKuratowskiObstructionMark> obstructionMarksByInternalVertex;

    int px = -1;
    int py = -1;
    BmRealExternalFacePosition pxPosition;
    BmRealExternalFacePosition pyPosition;

    // Directed real half-edges and original graph edge IDs of the marked
    // highest internal X-Y path, ordered from P_x to P_y.
    std::vector<int> xyPathHalfEdgeIds;
    std::vector<int> xyPathOriginalEdgeIds;

    // Internal vertex Z and the real embedded path from Z to R used by Minor D.
    int z = -1;
    std::vector<int> zToRootOriginalEdgeIds;
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
