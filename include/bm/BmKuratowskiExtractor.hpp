#pragma once

#include "bm/BmEmbeddingState.hpp"
#include "bm/BmKuratowskiExtractionContext.hpp"
#include "bm/BmKuratowskiMinorClassifier.hpp"
#include "bm/BmWalkdown.hpp"
#include "bm/PlanarityResult.hpp"

namespace bm {

class BmKuratowskiExtractor {
public:
    // Extracts and independently verifies a Kuratowski subdivision from the
    // preserved decision-core failure snapshot.
    static KuratowskiCertificate extract(
        const BmEmbeddingState& state,
        const BmWalkdownFailure& failure
    );

    // Retained for focused A/B unit tests.
    static KuratowskiCertificate extractInitialMinor(
        const BmEmbeddingState& state,
        const BmWalkdownFailure& failure
    );

private:
    static KuratowskiCertificate isolateMinorA(
        const BmEmbeddingState& state,
        const BmKuratowskiExtractionContext& context
    );

    static KuratowskiCertificate isolateMinorB(
        const BmEmbeddingState& state,
        const BmKuratowskiExtractionContext& context
    );

    static KuratowskiCertificate isolateMinorC(
        const BmEmbeddingState& state,
        const BmKuratowskiExtractionContext& context
    );

    static KuratowskiCertificate isolateMinorD(
        const BmEmbeddingState& state,
        const BmKuratowskiExtractionContext& context
    );

    static KuratowskiCertificate isolateMinorE(
        const BmEmbeddingState& state,
        const BmKuratowskiExtractionContext& context
    );
};

} // namespace bm
