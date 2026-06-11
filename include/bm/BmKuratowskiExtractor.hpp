#pragma once

#include "bm/BmEmbeddingState.hpp"
#include "bm/BmKuratowskiExtractionContext.hpp"
#include "bm/BmKuratowskiMinorClassifier.hpp"
#include "bm/BmWalkdown.hpp"
#include "bm/PlanarityResult.hpp"

namespace bm {

class BmKuratowskiExtractor {
public:
    // Extracts the reference Minor A and Minor B Kuratowski subdivisions.
    // C, D and E are added by the subsequent internal-X-Y-path stage.
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
};

} // namespace bm
