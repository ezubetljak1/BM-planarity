#pragma once

#include "bm/BmEmbeddingState.hpp"
#include "bm/BmKuratowskiExtractionContext.hpp"
#include "bm/BmPlanarityProfiling.hpp"

namespace bm {

enum class BmKuratowskiMinorType {
    Unknown,
    A,
    B,
    C,
    D,
    E
};

class BmKuratowskiMinorClassifier {
public:
    // Classifies the two cases that are determined directly after context
    // initialization. C, D and E require internal X-Y path analysis and are
    // intentionally returned as Unknown until the next isolation stage.
    static BmKuratowskiMinorType classifyInitial(
        const BmEmbeddingState& state,
        const BmKuratowskiExtractionContext& context
    );

    // Completes the reference-style C/D/E analysis when the initial A/B
    // tests do not decide the obstruction type. The context is enriched with
    // P_x, P_y, the highest X-Y path and, where applicable, Z.
    static BmKuratowskiMinorType classifyComplete(
        const BmEmbeddingState& state,
        BmKuratowskiExtractionContext& context,
        BmKuratowskiExtractionTimings* timings = nullptr
    );
};

} // namespace bm
