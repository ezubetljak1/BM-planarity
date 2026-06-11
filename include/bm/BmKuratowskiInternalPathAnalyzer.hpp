#pragma once

#include "bm/BmEmbeddingState.hpp"
#include "bm/BmKuratowskiExtractionContext.hpp"

namespace bm {

class BmKuratowskiInternalPathAnalyzer {
public:
    // Completes the reference-style context preparation for Minors C, D and E:
    // classify external-face vertices, find the highest internal X-Y path,
    // search for a Z-to-R path, and finally search for future pertinence below
    // the X-Y path.
    static void classifyExternalFaceVertices(
        const BmEmbeddingState& state,
        BmKuratowskiExtractionContext& context
    );

    static void findHighestXyPath(
        const BmEmbeddingState& state,
        BmKuratowskiExtractionContext& context
    );

    static void findZToRootPath(
        const BmEmbeddingState& state,
        BmKuratowskiExtractionContext& context
    );

    static int findFuturePertinentBelowXyPath(
        const BmEmbeddingState& state,
        const BmKuratowskiExtractionContext& context
    );
};

} // namespace bm
