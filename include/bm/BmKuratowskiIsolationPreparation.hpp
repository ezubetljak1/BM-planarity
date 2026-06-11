#pragma once

#include "bm/BmEmbeddingState.hpp"
#include "bm/BmKuratowskiExtractionContext.hpp"
#include "bm/BmKuratowskiMinorClassifier.hpp"
#include "bm/BmWalkdown.hpp"

namespace bm {

struct BmPreparedKuratowskiIsolation {
    BmEmbeddingState orientedState;
    BmKuratowskiExtractionContext context;
    BmKuratowskiMinorType minorType = BmKuratowskiMinorType::Unknown;
};

class BmKuratowskiIsolationPreparation {
public:
    // Isolation runs on a copy: lazy flip signs are normalized for real-face
    // traversal without mutating the decision-core failure snapshot.
    static BmPreparedKuratowskiIsolation prepare(
        const BmEmbeddingState& state,
        const BmWalkdownFailure& failure
    );
};

} // namespace bm
