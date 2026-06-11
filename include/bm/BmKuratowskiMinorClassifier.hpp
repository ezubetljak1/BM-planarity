#pragma once

#include "bm/BmEmbeddingState.hpp"
#include "bm/BmKuratowskiExtractionContext.hpp"

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
};

} // namespace bm
